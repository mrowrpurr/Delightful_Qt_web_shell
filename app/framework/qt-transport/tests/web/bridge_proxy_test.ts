import { test, expect, afterEach } from 'bun:test'
import { createWsConnection } from '@app/bridge/lib/transport/bridge-transport'

// ── Interface matching the REAL TodoBridge (request-object convention) ──

interface TodoBridge {
  addList(req: { name: string }): Promise<{ id: string; name: string; item_count: number; created_at: string }>
  listLists(): Promise<Array<{ id: string; name: string; item_count: number; created_at: string }>>
  listAdded: (cb: (data: any) => void) => () => void
}

// ── Helpers ─────────────────────────────────────────────────────────────

let servers: Array<{ stop(): void }> = []

afterEach(() => {
  for (const s of servers) s.stop()
  servers = []
})

/**
 * Validates that an incoming message conforms to the wire protocol:
 *   { bridge: string, method: string, args: [...], id: number }
 * Sends an error response if the shape is wrong.
 * Returns true if valid, false if rejected.
 */
function validateWireFormat(ws: any, data: any): boolean {
  if (typeof data !== 'object' || data === null) {
    ws.send(JSON.stringify({ id: data?.id ?? -1, error: 'Message must be an object' }))
    return false
  }
  if (typeof data.method !== 'string' || data.method.length === 0) {
    ws.send(JSON.stringify({ id: data.id ?? -1, error: 'Missing or invalid "method" field (must be non-empty string)' }))
    return false
  }
  if (!Array.isArray(data.args)) {
    ws.send(JSON.stringify({ id: data.id ?? -1, error: 'Missing or invalid "args" field (must be an array)' }))
    return false
  }
  if (typeof data.id !== 'number') {
    ws.send(JSON.stringify({ id: -1, error: 'Missing or invalid "id" field (must be a number)' }))
    return false
  }
  if (typeof data.bridge !== 'string' || data.bridge.length === 0) {
    ws.send(JSON.stringify({ id: data.id, error: 'Missing or invalid "bridge" field (must be non-empty string)' }))
    return false
  }
  return true
}

/**
 * Validates signalReady messages which have NO bridge field:
 *   { method: "appReady", args: [], id: number }
 */
function validateSignalReadyFormat(ws: any, data: any): boolean {
  if (typeof data !== 'object' || data === null) {
    ws.send(JSON.stringify({ id: data?.id ?? -1, error: 'Message must be an object' }))
    return false
  }
  if (data.method !== 'appReady') {
    ws.send(JSON.stringify({ id: data.id ?? -1, error: `Expected method "appReady", got "${data.method}"` }))
    return false
  }
  if (!Array.isArray(data.args) || data.args.length !== 0) {
    ws.send(JSON.stringify({ id: data.id ?? -1, error: 'appReady args must be an empty array' }))
    return false
  }
  if (typeof data.id !== 'number') {
    ws.send(JSON.stringify({ id: -1, error: 'Missing or invalid "id" field (must be a number)' }))
    return false
  }
  if ('bridge' in data) {
    ws.send(JSON.stringify({ id: data.id, error: 'appReady must NOT include a "bridge" field' }))
    return false
  }
  return true
}

function startServer(
  handler: (ws: any, data: any) => void,
  bridgeSignals: Record<string, string[]> = { todos: ['listAdded'] },
) {
  const server = Bun.serve({
    port: 0,
    fetch(req, server) {
      if (server.upgrade(req)) return
      return new Response('')
    },
    websocket: {
      message(ws: any, msg: string | Buffer) {
        const data = JSON.parse(msg as string)
        if (data.method === '__meta__') {
          // Validate __meta__ shape (no bridge field expected)
          if (typeof data.id !== 'number' || !Array.isArray(data.args)) {
            ws.send(JSON.stringify({ id: data.id ?? -1, error: 'Invalid __meta__ request' }))
            return
          }
          const bridges: Record<string, any> = {}
          for (const [name, signals] of Object.entries(bridgeSignals))
            bridges[name] = { signals }
          ws.send(JSON.stringify({ id: data.id, result: { bridges } }))
          return
        }
        handler(ws, data)
      },
    },
  })
  servers.push(server)
  return server
}

// ── Tests ───────────────────────────────────────────────────────────────

test('sends correct JSON-RPC message for a no-arg method', async () => {
  const received: any[] = []
  const server = startServer((ws, data) => {
    if (!validateWireFormat(ws, data)) return
    received.push(data)
    ws.send(JSON.stringify({ id: data.id, result: [] }))
  })

  const conn = await createWsConnection(`ws://localhost:${server.port}`)
  const bridge = conn.bridge<TodoBridge>('todos')
  await bridge.listLists()

  expect(received).toHaveLength(1)
  expect(received[0].bridge).toBe('todos')
  expect(received[0].method).toBe('listLists')
  expect(received[0].args).toEqual([])
  expect(typeof received[0].id).toBe('number')
})

test('sends request object for methods with parameters', async () => {
  const received: any[] = []
  const server = startServer((ws, data) => {
    if (!validateWireFormat(ws, data)) return
    received.push(data)
    ws.send(JSON.stringify({ id: data.id, result: { id: '1', name: 'Groceries', item_count: 0, created_at: '2026-01-01' } }))
  })

  const conn = await createWsConnection(`ws://localhost:${server.port}`)
  const bridge = conn.bridge<TodoBridge>('todos')
  await bridge.addList({ name: 'Groceries' })

  expect(received[0].bridge).toBe('todos')
  expect(received[0].method).toBe('addList')
  expect(received[0].args).toEqual([{ name: 'Groceries' }])
})

test('resolves with the result from the server', async () => {
  const server = startServer((ws, data) => {
    if (!validateWireFormat(ws, data)) return
    ws.send(JSON.stringify({
      id: data.id,
      result: [
        { id: '1', name: 'Groceries', item_count: 2, created_at: '2026-01-01' },
      ],
    }))
  })

  const conn = await createWsConnection(`ws://localhost:${server.port}`)
  const bridge = conn.bridge<TodoBridge>('todos')
  const lists = await bridge.listLists()

  expect(lists).toHaveLength(1)
  expect(lists[0].name).toBe('Groceries')
  expect(lists[0].item_count).toBe(2)
})

test('rejects when server returns an error', async () => {
  const server = startServer((ws, data) => {
    if (!validateWireFormat(ws, data)) return
    ws.send(JSON.stringify({ id: data.id, error: 'Not found' }))
  })

  const conn = await createWsConnection(`ws://localhost:${server.port}`)
  const bridge = conn.bridge<TodoBridge>('todos')
  try {
    await bridge.addList({ name: 'nonexistent' })
    throw new Error('should have thrown')
  } catch (e: any) {
    expect(e.message).toBe('Not found')
  }
})

test('increments request IDs for concurrent calls', async () => {
  const received: any[] = []
  const server = startServer((ws, data) => {
    if (!validateWireFormat(ws, data)) return
    received.push(data)
    ws.send(JSON.stringify({ id: data.id, result: [] }))
  })

  const conn = await createWsConnection(`ws://localhost:${server.port}`)
  const bridge = conn.bridge<TodoBridge>('todos')
  await Promise.all([bridge.listLists(), bridge.listLists()])

  const ids = received.map(r => r.id)
  expect(ids[0]).not.toBe(ids[1])
})

test('listAdded fires when server pushes an event', async () => {
  let sendEvent: ((payload: any) => void) | null = null
  const server = startServer((ws, data) => {
    if (!validateWireFormat(ws, data)) return
    ws.send(JSON.stringify({ id: data.id, result: [] }))
    sendEvent = (payload: any) => ws.send(JSON.stringify({ bridge: 'todos', event: 'listAdded', args: payload }))
  })

  const conn = await createWsConnection(`ws://localhost:${server.port}`)
  const bridge = conn.bridge<TodoBridge>('todos')

  // Make an initial call to capture the ws reference in sendEvent
  await bridge.listLists()

  // Register listener
  let eventData: any = null
  bridge.listAdded((data) => { eventData = data })

  // Push the event
  const payload = { id: '2', name: 'New List', item_count: 0, created_at: '2026-05-16' }
  sendEvent!(payload)

  // Give it a moment to propagate
  await new Promise(r => setTimeout(r, 50))
  expect(eventData).toEqual(payload)
})

test('listAdded cleanup removes the listener', async () => {
  let sendEvent: ((payload: any) => void) | null = null
  const server = startServer((ws, data) => {
    if (!validateWireFormat(ws, data)) return
    ws.send(JSON.stringify({ id: data.id, result: [] }))
    sendEvent = (payload: any) => ws.send(JSON.stringify({ bridge: 'todos', event: 'listAdded', args: payload }))
  })

  const conn = await createWsConnection(`ws://localhost:${server.port}`)
  const bridge = conn.bridge<TodoBridge>('todos')
  await bridge.listLists()

  let count = 0
  const cleanup = bridge.listAdded(() => { count++ })

  sendEvent!(null)
  await new Promise(r => setTimeout(r, 50))
  expect(count).toBe(1)

  // Remove listener
  cleanup()

  sendEvent!(null)
  await new Promise(r => setTimeout(r, 50))
  expect(count).toBe(1) // should NOT have incremented
})

test('signalReady sends appReady with no bridge field and resolves', async () => {
  const received: any[] = []
  const server = startServer((ws, data) => {
    // signalReady has no bridge field — use the dedicated validator
    if (!validateSignalReadyFormat(ws, data)) return
    received.push(data)
    ws.send(JSON.stringify({ id: data.id, result: {} }))
  })

  const conn = await createWsConnection(`ws://localhost:${server.port}`)
  await conn.signalReady()

  expect(received).toHaveLength(1)
  expect(received[0].method).toBe('appReady')
  expect(received[0].args).toEqual([])
  expect(received[0]).not.toHaveProperty('bridge')
  expect(typeof received[0].id).toBe('number')
})

test('mock rejects messages with invalid wire format', async () => {
  // This test verifies our mock actually rejects bad shapes.
  // We send a raw message missing the "bridge" field to a handler that validates.
  const received: any[] = []
  const server = startServer((ws, data) => {
    if (!validateWireFormat(ws, data)) return
    received.push(data)
    ws.send(JSON.stringify({ id: data.id, result: 'ok' }))
  })

  // Connect and manually send a malformed message through a raw WebSocket
  const ws = new WebSocket(`ws://localhost:${server.port}`)
  await new Promise<void>((resolve) => { ws.onopen = () => resolve() })

  // Consume the __meta__ response (the server sends it back automatically)
  // but first we need to send __meta__ since that's what the server expects first
  ws.send(JSON.stringify({ method: '__meta__', args: [], id: 0 }))
  await new Promise(r => setTimeout(r, 50))

  // Send a message missing the "bridge" field — should be rejected
  ws.send(JSON.stringify({ method: 'addList', args: [{ name: 'X' }], id: 1 }))

  await new Promise(r => setTimeout(r, 50))
  // The mock should NOT have added this to received because validation failed
  expect(received).toHaveLength(0)

  ws.close()
})
