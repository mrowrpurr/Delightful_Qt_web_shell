// Auto-generated from C++ def_type structs — do not edit.
// Regenerate: xmake run generate-dtos

export interface AddListRequest {
  name: string
}

export interface GetListRequest {
  list_id: string
}

export interface AddItemRequest {
  list_id: string
  text: string
}

export interface ToggleItemRequest {
  item_id: string
}

export interface DeleteListRequest {
  list_id: string
}

export interface DeleteItemRequest {
  item_id: string
}

export interface RenameListRequest {
  list_id: string
  new_name: string
}

export interface SearchRequest {
  query: string
}

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
