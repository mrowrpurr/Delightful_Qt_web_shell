import { getBridge } from '../transport/bridge'

// Re-export generated DTOs so consumers can import from the bridge file
export type {
  AddListRequest,
  GetListRequest,
  AddItemRequest,
  ToggleItemRequest,
  DeleteListRequest,
  DeleteItemRequest,
  RenameListRequest,
  SearchRequest,
  TodoList,
  TodoItem,
  ListDetail,
} from '../dtos/todos.dtos'
export type { OkResponse } from '../dtos/framework.dtos'

import type {
  AddListRequest,
  GetListRequest,
  AddItemRequest,
  ToggleItemRequest,
  DeleteListRequest,
  DeleteItemRequest,
  RenameListRequest,
  SearchRequest,
  TodoList,
  TodoItem,
  ListDetail,
} from '../dtos/todos.dtos'
import type { OkResponse } from '../dtos/framework.dtos'

// TypeScript interface for the TodoBridge C++ bridge.
// CRUD over todo lists + items, with signals for live updates.
// DTOs are generated from C++ — run `xmake run generate-dtos` after changes.

export interface TodoBridge {
  listLists(): Promise<TodoList[]>
  getList(req: GetListRequest): Promise<ListDetail>
  addList(req: AddListRequest): Promise<TodoList>
  addItem(req: AddItemRequest): Promise<TodoItem>
  toggleItem(req: ToggleItemRequest): Promise<TodoItem>
  deleteList(req: DeleteListRequest): Promise<OkResponse>
  deleteItem(req: DeleteItemRequest): Promise<OkResponse>
  renameList(req: RenameListRequest): Promise<TodoList>
  search(req: SearchRequest): Promise<TodoItem[]>
  listAdded(callback: (data: TodoList) => void): () => void
  listRenamed(callback: (data: TodoList) => void): () => void
  listDeleted(callback: (data: DeleteListRequest) => void): () => void
  itemAdded(callback: (data: TodoItem) => void): () => void
  itemToggled(callback: (data: TodoItem) => void): () => void
  itemDeleted(callback: (data: DeleteItemRequest) => void): () => void
}

export async function getTodoBridge(): Promise<TodoBridge> {
  return getBridge<TodoBridge>('todos')
}
