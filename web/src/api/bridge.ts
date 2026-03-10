import { createQtBridge, createWsBridge } from './bridge-transport'

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
  appReady(): Promise<void>
  dataChanged(callback: () => void): () => void
}

// ── Bridge singleton ─────────────────────────────────────────────────
// Auto-detects the right transport. You never need to think about this.

let _bridge: Promise<TodoBridge> | null = null

export function createBridge(): Promise<TodoBridge> {
  if (!_bridge) {
    if (window.qt?.webChannelTransport && window.QWebChannel)
      _bridge = createQtBridge<TodoBridge>()
    else {
      const wsUrl = import.meta.env.VITE_BRIDGE_WS_URL || 'ws://localhost:9876'
      _bridge = createWsBridge<TodoBridge>(wsUrl)
    }
  }
  return _bridge
}
