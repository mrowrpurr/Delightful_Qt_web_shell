import { createQtConnection, createWsConnection, type BridgeConnection } from './bridge-transport'

// ── Domain types ──────────────────────────────────────────────────────
// Snake_case field names match the C++ structs and JSON wire format.
// No mapping layer = zero boilerplate when adding new fields.

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
// Every bridge method returns a Promise. Add methods here and on the
// C++ side (Q_INVOKABLE) — the plumbing connects them automatically.

export interface TodoBridge {
  listLists(): Promise<TodoList[]>
  getList(listId: string): Promise<ListDetail>
  addList(name: string): Promise<TodoList>
  addItem(listId: string, text: string): Promise<TodoItem>
  toggleItem(itemId: string): Promise<TodoItem>
  search(query: string): Promise<TodoItem[]>
  dataChanged(callback: () => void): () => void
}

// ── Connection singleton ────────────────────────────────────────────
// Auto-detects the right transport. You never need to think about this.

let _connection: Promise<BridgeConnection> | null = null

function getConnection(): Promise<BridgeConnection> {
  if (!_connection) {
    if (window.qt?.webChannelTransport && window.QWebChannel)
      _connection = createQtConnection()
    else {
      const wsUrl = import.meta.env.VITE_BRIDGE_WS_URL || 'ws://localhost:9876'
      _connection = createWsConnection(wsUrl)
    }
  }
  return _connection
}

// ── Public API ──────────────────────────────────────────────────────

export async function useBridge<T extends object>(name: string): Promise<T> {
  const conn = await getConnection()
  return conn.bridge<T>(name)
}

export async function signalReady(): Promise<void> {
  const conn = await getConnection()
  return conn.signalReady()
}
