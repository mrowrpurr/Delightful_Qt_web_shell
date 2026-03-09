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

export function createWsBridge<T extends object>(url: string): T {
  let ws: WebSocket | null = null
  let nextId = 0
  const pending = new Map<number, { resolve: (v: any) => void; reject: (e: Error) => void }>()
  const eventListeners: Record<string, Array<() => void>> = {}
  const signals = new Set<string>()

  const ready = new Promise<void>((resolve, reject) => {
    ws = new WebSocket(url)
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
    ws.onopen = () => {
      // Query the bridge for its signal names before resolving ready
      const id = nextId++
      pending.set(id, {
        resolve: (result: any) => {
          for (const name of result?.signals ?? [])
            signals.add(name)
          resolve()
        },
        reject,
      })
      ws!.send(JSON.stringify({ method: '__meta__', args: [], id }))
    }
  })

  return new Proxy({} as T, {
    get(_, prop) {
      if (typeof prop === 'symbol' || prop === 'then' || prop === 'toJSON') return undefined
      const name = prop as string

      // Post-meta: we know this is a signal
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

      // Pre-meta signal subscription: register immediately, return sync cleanup.
      // After meta resolves the fast path above handles future subscriptions.
      return (...args: any[]) => {
        if (typeof args[0] === 'function') {
          const callback = args[0] as () => void
          const listeners = eventListeners[name] ??= []
          listeners.push(callback)
          return () => {
            const idx = listeners.indexOf(callback)
            if (idx >= 0) listeners.splice(idx, 1)
          }
        }
        // Method call (waits for ready, which includes the meta query)
        return (async () => {
          await ready
          return new Promise((resolve, reject) => {
            const id = nextId++
            pending.set(id, { resolve, reject })
            ws!.send(JSON.stringify({ method: name, args, id }))
          })
        })()
      }
    },
  }) as T
}

export function createQtBridge<T extends object>(): T {
  let bridge: any = null
  const signals = new Set<string>()

  const ready = new Promise<void>((resolve) => {
    new window.QWebChannel!(window.qt!.webChannelTransport, (channel) => {
      bridge = channel.objects.bridge
      // Discover signals from the QWebChannel bridge object
      for (const key of Object.keys(bridge)) {
        if (bridge[key]?.connect && bridge[key]?.disconnect)
          signals.add(key)
      }
      resolve()
    })
  })

  return new Proxy({} as T, {
    get(_, prop) {
      if (typeof prop === 'symbol' || prop === 'then' || prop === 'toJSON') return undefined
      const name = prop as string

      // Post-ready: we know this is a signal
      if (signals.has(name)) {
        return (callback: () => void) => {
          bridge?.[name]?.connect(callback)
          return () => {
            bridge?.[name]?.disconnect(callback)
          }
        }
      }

      // Method call (waits for ready, which populates signals)
      return async (...args: any[]) => {
        await ready
        // Check again after ready — might be a signal accessed before ready
        if (signals.has(name) && typeof args[0] === 'function') {
          const callback = args[0] as () => void
          bridge?.[name]?.connect(callback)
          return () => {
            bridge?.[name]?.disconnect(callback)
          }
        }
        return new Promise((resolve, reject) => {
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
      }
    },
  }) as T
}
