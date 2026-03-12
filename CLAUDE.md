# CLAUDE.md — Agent Onboarding

## 🏗️ Build & Run

```bash
xmake build desktop              # Build everything (web + C++). Skips web if unchanged.
xmake build dev-server            # Build the headless WS backend (for browser-mode dev)
xmake run dev-web                 # Start Vite dev server (HMR on :5173)
xmake run dev-desktop             # Build + run desktop with --dev (HMR) + CDP on :9222
xmake run start-desktop           # Launch app in background with CDP on :9222
xmake run stop-desktop            # Kill background app
```

## 🧪 Testing

```bash
xmake run test-todo-store         # C++ unit tests (Catch2) — instant
xmake run test-bun                # Bridge proxy unit tests — < 1s
xmake run test-browser            # Playwright e2e (browser) — ~5s
xmake run test-desktop            # Playwright e2e (real Qt app) — ~15s
xmake run test-pywinauto          # Native Qt tests (pywinauto) — requires running app
xmake run test-all                # Catch2 + Bun + browser e2e
```

Always run tests before AND after changes. That's your baseline.

## 🔧 Adding a Bridge Method

Three files for a new method on an existing bridge:
1. `lib/todos/include/todo_store.hpp` — pure C++ logic
2. `lib/web-bridge/include/bridge.hpp` — `Q_INVOKABLE` wrapper
3. `web/src/api/bridge.ts` — TypeScript interface

### Adding a NEW bridge (new domain)

Five files:
1. New C++ header with QObject + Q_INVOKABLE methods
2. New xmake target in `lib/your-bridge/xmake.lua`
3. Register in `desktop/src/main.cpp`: `shell->addBridge("name", new YourBridge)`
4. **Register in `tests/helpers/dev-server/src/test_server.cpp` too!** ← easy to forget
5. TypeScript interface in `web/src/api/bridge.ts`

**Also:** add the .hpp to `add_files` in both `desktop/xmake.lua` and `tests/helpers/dev-server/xmake.lua` — Qt MOC needs it. Missing this → opaque vtable linker errors.

## ⚡ Bridge Capabilities

**Parameter types:** QString, int, double, bool, QJsonObject, QJsonArray
**Return types:** QJsonObject, QJsonArray, QString, int, double, bool, void
**Max parameters:** 10
**Signals:** parameterless signals are auto-forwarded to JS clients

## 🚫 Gotchas

- **cdp-mcp must run under Node, not Bun** — Bun's ws polyfill breaks CDP WebSocket
- **playwright-core needs patching for QtWebEngine** — `Browser.setDownloadBehavior` unsupported. Root uses `patchedDependencies`, cdp-mcp has postinstall script
- **`signalReady()` in App.tsx must not be removed** — it tells Qt that React mounted. Without it: infinite spinner, no error
- **Web build is cached** — `desktop/xmake.lua` skips rebuild when `web/src/` hasn't changed. Touch a web file to force rebuild.

## 📁 Key Files

| What | Where |
|------|-------|
| C++ domain logic | `lib/todos/include/todo_store.hpp` |
| Bridge (QObject wrapper) | `lib/web-bridge/include/bridge.hpp` |
| Bridge infrastructure | `lib/web-shell/include/expose_as_ws.hpp` |
| Desktop entry point | `desktop/src/main.cpp` |
| Dev server entry point | `tests/helpers/dev-server/src/test_server.cpp` |
| React app | `web/src/App.tsx` |
| Bridge TS interface | `web/src/api/bridge.ts` |
| Bridge transport | `web/src/api/bridge-transport.ts` |
| MCP server (cdp-mcp) | `tools/cdp-mcp/server.ts` |
| Playwright tests | `tests/playwright/` |
| pywinauto tests | `tests/pywinauto/` |

## 📚 Guides

- `TUTORIAL.md` — how to add features (bridge walkthrough)
- `TESTING_GUIDE.md` — comprehensive testing guide (agent-first)
- `ARCHITECTURE.md` — system architecture overview
