import { getBridge } from '../transport/bridge'

// Re-export generated DTOs so consumers can import from the bridge file
export type {
  CopyToClipboardRequest,
  ReadClipboardResponse,
  OpenFileChooserRequest,
  FileChooserResponse,
  ListFolderRequest,
  ListFolderResponse,
  FolderEntry,
  GlobFolderRequest,
  GlobFolderResponse,
  ReadTextFileRequest,
  ReadTextFileResponse,
  ReadFileBytesRequest,
  ReadFileBytesResponse,
  WriteTextFileRequest,
  OpenFileHandleRequest,
  OpenFileHandleResponse,
  ReadFileChunkRequest,
  ReadFileChunkResponse,
  CloseFileHandleRequest,
  StringListResponse,
} from '../dtos/system.dtos'
export type { OkResponse } from '../dtos/framework.dtos'

import type {
  CopyToClipboardRequest,
  ReadClipboardResponse,
  OpenFileChooserRequest,
  FileChooserResponse,
  ListFolderRequest,
  ListFolderResponse,
  GlobFolderRequest,
  GlobFolderResponse,
  ReadTextFileRequest,
  ReadTextFileResponse,
  ReadFileBytesRequest,
  ReadFileBytesResponse,
  WriteTextFileRequest,
  OpenFileHandleRequest,
  OpenFileHandleResponse,
  ReadFileChunkRequest,
  ReadFileChunkResponse,
  CloseFileHandleRequest,
  StringListResponse,
} from '../dtos/system.dtos'
import type { OkResponse } from '../dtos/framework.dtos'

// TypeScript interface for the SystemBridge C++ bridge.
// Desktop capabilities: clipboard, file I/O, file drop, etc.
// DTOs are generated from C++ — run `xmake run generate-dtos` after changes.

export interface SystemBridge {
  // ── Clipboard ──────────────────────────────────────────
  copyToClipboard(req: CopyToClipboardRequest): Promise<OkResponse>
  readClipboard(): Promise<ReadClipboardResponse>

  // ── File choosers ──────────────────────────────────────
  openFileChooser(req: OpenFileChooserRequest): Promise<FileChooserResponse>
  openFolderChooser(): Promise<FileChooserResponse>

  // ── Directory listing ──────────────────────────────────
  listFolder(req: ListFolderRequest): Promise<ListFolderResponse>
  globFolder(req: GlobFolderRequest): Promise<GlobFolderResponse>

  // ── Simple file reads/writes ──────────────────────────────
  readTextFile(req: ReadTextFileRequest): Promise<ReadTextFileResponse>
  readFileBytes(req: ReadFileBytesRequest): Promise<ReadFileBytesResponse>
  writeTextFile(req: WriteTextFileRequest): Promise<OkResponse>

  // ── File handles (streaming) ───────────────────────────
  openFileHandle(req: OpenFileHandleRequest): Promise<OpenFileHandleResponse>
  readFileChunk(req: ReadFileChunkRequest): Promise<ReadFileChunkResponse>
  closeFileHandle(req: CloseFileHandleRequest): Promise<OkResponse>

  // ── Args from CLI / URL protocol / other instance ───────
  getAppLaunchArgs(): Promise<StringListResponse>
  appLaunchArgsReceived(callback: (data: StringListResponse) => void): () => void

  // ── File drop ──────────────────────────────────────────
  getDroppedFiles(): Promise<string[]>
  filesDropped(callback: (data: StringListResponse) => void): () => void

  // ── Save ──────────────────────────────────────────────────
  saveRequested(callback: () => void): () => void

  // ── Native dialogs ─────────────────────────────────────
  openDialog(): Promise<OkResponse>
  openDialogRequested(callback: () => void): () => void
}

export async function getSystemBridge(): Promise<SystemBridge> {
  return getBridge<SystemBridge>('system')
}
