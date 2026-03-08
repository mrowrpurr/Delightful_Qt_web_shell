// ── Domain types ──────────────────────────────────────────────────────

export interface TodoList {
  id: string
  name: string
  item_count: number
  created_at: string
}

export interface TodoItem {
  id: string
  list_id: string
  text: string
  done: boolean
  created_at: string
}

export interface ListDetail {
  list: TodoList
  items: TodoItem[]
}

// ── Bridge interface ──────────────────────────────────────────────────
// Every bridge method returns a Promise. Same interface whether backed
// by QWebChannel (production), WebSocket (dev/test), or anything else.

export interface TodoBridge {
  listLists(): Promise<TodoList[]>
  getList(listId: string): Promise<ListDetail>
  addList(name: string): Promise<TodoList>
  addItem(listId: string, text: string): Promise<TodoItem>
  toggleItem(itemId: string): Promise<TodoItem>
  search(query: string): Promise<TodoItem[]>
  onDataChanged(callback: () => void): () => void
}

// ── WebSocket bridge (dev / test / Playwright) ────────────────────────
// A Proxy that turns any interface into WebSocket JSON-RPC calls.
// Zero per-method code. The interface IS the implementation.

export function createWsBridge<T>(url: string): T {
  let ws: WebSocket | null = null
  let nextId = 0
  const pending = new Map<number, { resolve: (v: any) => void; reject: (e: Error) => void }>()
  const eventListeners: Record<string, Array<() => void>> = {}

  const ready = new Promise<void>((resolve, reject) => {
    ws = new WebSocket(url)
    ws.onopen = () => resolve()
    ws.onerror = () => reject(new Error(`WebSocket connection failed: ${url}`))
    ws.onmessage = (e) => {
      const msg = JSON.parse(e.data)
      if (msg.id !== undefined && pending.has(msg.id)) {
        const p = pending.get(msg.id)!
        pending.delete(msg.id)
        if (msg.error) p.reject(new Error(msg.error))
        else p.resolve(msg.result)
      }
      if (msg.event) {
        eventListeners[msg.event]?.forEach(cb => cb())
      }
    }
  })

  return new Proxy({} as T, {
    get(_, prop: string) {
      if (prop === 'onDataChanged') {
        return (callback: () => void) => {
          const listeners = eventListeners['dataChanged'] ??= []
          listeners.push(callback)
          return () => {
            const idx = listeners.indexOf(callback)
            if (idx >= 0) listeners.splice(idx, 1)
          }
        }
      }
      return async (...args: any[]) => {
        await ready
        return new Promise((resolve, reject) => {
          const id = nextId++
          pending.set(id, { resolve, reject })
          ws!.send(JSON.stringify({ method: prop, args, id }))
        })
      }
    },
  }) as T
}

// ── Qt WebChannel bridge (production) ─────────────────────────────────

declare global {
  interface Window {
    qt?: { webChannelTransport: unknown }
    QWebChannel?: new (
      transport: unknown,
      callback: (channel: { objects: Record<string, any> }) => void
    ) => void
  }
}

class QtBridge implements TodoBridge {
  private bridge: any = null
  private ready: Promise<void>

  constructor() {
    this.ready = new Promise<void>((resolve) => {
      new window.QWebChannel!(window.qt!.webChannelTransport, (channel) => {
        this.bridge = channel.objects.bridge
        resolve()
      })
    })
  }

  private async call<T>(method: string, ...args: any[]): Promise<T> {
    await this.ready
    return new Promise((resolve, reject) => {
      this.bridge[method](...args, (result: string) => {
        try {
          const data = JSON.parse(result)
          if (data.error) reject(new Error(data.error))
          else resolve(data)
        } catch (e) {
          reject(e)
        }
      })
    })
  }

  async listLists(): Promise<TodoList[]> { return this.call('listLists') }
  async getList(listId: string): Promise<ListDetail> { return this.call('getList', listId) }
  async addList(name: string): Promise<TodoList> { return this.call('addList', name) }
  async addItem(listId: string, text: string): Promise<TodoItem> { return this.call('addItem', listId, text) }
  async toggleItem(itemId: string): Promise<TodoItem> { return this.call('toggleItem', itemId) }
  async search(query: string): Promise<TodoItem[]> { return this.call('search', query) }

  onDataChanged(callback: () => void): () => void {
    let disconnected = false
    this.ready.then(() => {
      if (disconnected) return
      this.bridge?.dataChanged?.connect(callback)
    })
    return () => {
      disconnected = true
      this.bridge?.dataChanged?.disconnect(callback)
    }
  }
}

// ── Auto-detect (singleton) ───────────────────────────────────────────

let _bridge: TodoBridge | null = null

export function createBridge(): TodoBridge {
  if (!_bridge) {
    if (window.qt?.webChannelTransport && window.QWebChannel) {
      _bridge = new QtBridge()
    } else {
      const wsUrl = import.meta.env.VITE_BRIDGE_WS_URL || 'ws://localhost:9876'
      _bridge = createWsBridge<TodoBridge>(wsUrl)
    }
  }
  return _bridge
}
