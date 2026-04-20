// Request/response DTOs for SystemBridge methods.

#pragma once

#include <def_type.hpp>

// ── Clipboard ────────────────────────────────────────────
struct CopyToClipboardRequest   { std::string text; };
struct ReadClipboardResponse    { std::string text; };

// ── File choosers ────────────────────────────────────────
struct OpenFileChooserRequest   { std::string filter; };
struct OpenFolderChooserRequest {};
struct FileChooserResponse      { std::string path; bool cancelled = false; };

// ── Directory listing ────────────────────────────────────
struct ListFolderRequest        { std::string path; };
struct GlobFolderRequest        { std::string path; std::string pattern; bool recursive = false; };

// ── File I/O ─────────────────────────────────────────────
struct ReadTextFileRequest      { std::string path; };
struct ReadTextFileResponse     { std::string text; };
struct ReadFileBytesRequest     { std::string path; };
struct ReadFileBytesResponse    { std::string data; };
struct WriteTextFileRequest     { std::string path; std::string text; };

// ── File handles (streaming) ─────────────────────────────
struct OpenFileHandleRequest    { std::string path; };
struct OpenFileHandleResponse   { std::string handle; int64_t size; };
struct ReadFileChunkRequest     { std::string handle; int64_t offset; int64_t length; };
struct ReadFileChunkResponse    { std::string data; int64_t bytesRead; };
struct CloseFileHandleRequest   { std::string handle; };

// ── Theme control ────────────────────────────────────────
struct SetQtThemeRequest        { std::string displayName; bool isDark; };
struct GetQtThemeResponse       { std::string displayName; bool isDark; };
struct GetQtThemeFilePathResponse { std::string path; bool embedded = false; };
