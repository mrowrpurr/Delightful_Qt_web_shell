// Auto-generated from C++ def_type structs — do not edit.
// Regenerate: xmake run generate-dtos

export interface CopyToClipboardRequest {
  text: string
}

export interface ReadClipboardResponse {
  text: string
}

export interface OpenFileChooserRequest {
  filter: string
}

export interface OpenFolderChooserRequest {
}

export interface FileChooserResponse {
  path: string
  cancelled: boolean
}

export interface ListFolderRequest {
  path: string
}

export interface FolderEntry {
  name: string
  isDir: boolean
  size: number
}

export interface ListFolderResponse {
  entries: FolderEntry[]
}

export interface GlobFolderRequest {
  path: string
  pattern: string
  recursive: boolean
}

export interface GlobFolderResponse {
  paths: string[]
}

export interface ReadTextFileRequest {
  path: string
}

export interface ReadTextFileResponse {
  text: string
}

export interface ReadFileBytesRequest {
  path: string
}

export interface ReadFileBytesResponse {
  data: string
}

export interface WriteTextFileRequest {
  path: string
  text: string
}

export interface OpenFileHandleRequest {
  path: string
}

export interface OpenFileHandleResponse {
  handle: string
  size: number
}

export interface ReadFileChunkRequest {
  handle: string
  offset: number
  length: number
}

export interface ReadFileChunkResponse {
  data: string
  bytesRead: number
}

export interface CloseFileHandleRequest {
  handle: string
}

export interface StringListResponse {
  items: string[]
}
