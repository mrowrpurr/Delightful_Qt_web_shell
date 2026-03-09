// Internal transport implementations. You don't need to touch this file.
// See bridge.ts for the public API.

function eventNameFromProp(prop: string): string | null {
  const match = prop.match(/^on([A-Z].*)$/)
  if (!match) return null
  return match[1][0].toLowerCase() + match[1].slice(1)
}

declare global {
  interface Window {
    qt?: { webChannelTransport: unknown }
    QWebChannel?: new (
      transport: unknown,
      callback: (channel: { objects: Record<string, any> }) => void
    ) => void
  }
}

export function createWsBridge<T extends object>(url: string): T {
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
    get(_, prop) {
      if (typeof prop === 'symbol' || prop === 'then' || prop === 'toJSON') return undefined
      const eventName = eventNameFromProp(prop)
      if (eventName) {
        return (callback: () => void) => {
          const listeners = eventListeners[eventName] ??= []
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

export function createQtBridge<T extends object>(): T {
  let bridge: any = null
  const ready = new Promise<void>((resolve) => {
    new window.QWebChannel!(window.qt!.webChannelTransport, (channel) => {
      bridge = channel.objects.bridge
      resolve()
    })
  })

  return new Proxy({} as T, {
    get(_, prop) {
      if (typeof prop === 'symbol' || prop === 'then' || prop === 'toJSON') return undefined
      const eventName = eventNameFromProp(prop)
      if (eventName) {
        return (callback: () => void) => {
          let disconnected = false
          ready.then(() => {
            if (disconnected) return
            bridge?.[eventName]?.connect(callback)
          })
          return () => {
            disconnected = true
            bridge?.[eventName]?.disconnect(callback)
          }
        }
      }
      return async (...args: any[]) => {
        await ready
        return new Promise((resolve, reject) => {
          bridge[prop](...args, (result: string) => {
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
    },
  }) as T
}
