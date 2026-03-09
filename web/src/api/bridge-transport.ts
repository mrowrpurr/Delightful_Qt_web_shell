// Internal transport implementations. You don't need to touch this file.
// See bridge.ts for the public API.

declare global {
  interface Window {
    qt?: { webChannelTransport: unknown }
    QWebChannel?: new (
      transport: unknown,
      callback: (channel: { objects: Record<string, any> }) => void
    ) => void
  }
}

export async function createWsBridge<T extends object>(url: string): Promise<T> {
  let nextId = 0
  const pending = new Map<number, { resolve: (v: any) => void; reject: (e: Error) => void }>()
  const eventListeners: Record<string, Array<() => void>> = {}
  const signals = new Set<string>()

  const ws = await new Promise<WebSocket>((resolve, reject) => {
    const socket = new WebSocket(url)
    socket.onerror = () => reject(new Error(`WebSocket connection failed: ${url}`))
    socket.onopen = () => resolve(socket)
  })

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

  // Query the bridge for its signal names before returning
  const meta = await new Promise<any>((resolve, reject) => {
    const id = nextId++
    pending.set(id, { resolve, reject })
    ws.send(JSON.stringify({ method: '__meta__', args: [], id }))
  })
  for (const name of meta?.signals ?? [])
    signals.add(name)

  return new Proxy({} as T, {
    get(_, prop) {
      if (typeof prop === 'symbol' || prop === 'then' || prop === 'toJSON') return undefined
      const name = prop as string

      if (signals.has(name)) {
        return (callback: () => void) => {
          const listeners = eventListeners[name] ??= []
          listeners.push(callback)
          return () => {
            const idx = listeners.indexOf(callback)
            if (idx >= 0) listeners.splice(idx, 1)
          }
        }
      }

      return (...args: any[]) =>
        new Promise((resolve, reject) => {
          const id = nextId++
          pending.set(id, { resolve, reject })
          ws.send(JSON.stringify({ method: name, args, id }))
        })
    },
  }) as T
}

export async function createQtBridge<T extends object>(): Promise<T> {
  const signals = new Set<string>()

  const bridge = await new Promise<any>((resolve) => {
    new window.QWebChannel!(window.qt!.webChannelTransport, (channel) => {
      resolve(channel.objects.bridge)
    })
  })

  for (const key of Object.keys(bridge)) {
    if (bridge[key]?.connect && bridge[key]?.disconnect)
      signals.add(key)
  }

  return new Proxy({} as T, {
    get(_, prop) {
      if (typeof prop === 'symbol' || prop === 'then' || prop === 'toJSON') return undefined
      const name = prop as string

      if (signals.has(name)) {
        return (callback: () => void) => {
          bridge[name].connect(callback)
          return () => {
            bridge[name].disconnect(callback)
          }
        }
      }

      return (...args: any[]) =>
        new Promise((resolve, reject) => {
          bridge[name](...args, (result: any) => {
            try {
              const data = typeof result === 'string' ? JSON.parse(result) : result
              if (data?.error) reject(new Error(data.error))
              else resolve(data)
            } catch (e) {
              reject(e)
            }
          })
        })
    },
  }) as T
}
