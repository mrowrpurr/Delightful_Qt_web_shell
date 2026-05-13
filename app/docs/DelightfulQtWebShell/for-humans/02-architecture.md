# Architecture

## The Big Picture

One React UI, one domain library, two deployment targets:

```
                                ┌──────────────────┐
                          ┌────►│ BridgeChannel    │
                          │     │ Adapter (QObject)│
                          │     │ dispatch()       │    ┌──────────────┐
 React (Vite)             │     └──────────────────┘    │ TodoBridge   │
 ┌──────────┐    transport│            │                │ extends      │
 │  UI      │◄────────────┤            └───────────────►│ app_shell::  │───► TodoStore
 │  bridge  │  auto-detect│                             │ Bridge       │     (pure C++)
 │  proxy   │             │     ┌──────────────────┐    │              │
 └──────────┘             └────►│ WasmBridgeWrapper│───►│ One class.   │
                                │ (generic Embind) │    │ Both targets.│
      transport:                └──────────────────┘    └──────────────┘
      ├── QWebChannel
      ├── WebSocket
      └── WASM (Embind)
```

**The bridge is a controller.** `TodoStore` is the model. There is **one** bridge class per domain area — pure C++, extending `app_shell::Bridge`, with **no Qt types, no Emscripten types**. The transport layer (QWebChannel adapter, WebSocket server, or WASM wrapper) handles serialization at the edges. React is the view. The transport is invisible.

The app has three layers you touch:

1. **Domain logic** (`<repo>/lib/<domain>/`) — Pure C++, no Qt, no Emscripten. Testable in isolation with Catch2. Compiled for both desktop and WASM.
2. **Bridge** (`<repo>/app/bridges/<domain>/`) — A class extending `app_shell::Bridge`. Methods take plain C++ request DTOs and return plain C++ structs. No `Q_INVOKABLE`, no `QVariant`, no `emscripten::val`. One bridge class serves both desktop and WASM.
3. **TypeScript interface** (`@app/bridge/lib/bridges/<name>-bridge.ts`) — Declares the methods and signals your bridge exposes. Shared by both targets — React doesn't know which bridge it's talking to.

And two layers you don't touch:

- **Framework** (`<repo>/app/framework/`) — `app_shell::Bridge` base, `BridgeRegistry` (pure C++) + `AppLifecycle` (Qt QObject), Qt and WASM transport adapters. You call `registry.addBridge("name", bridge)` and never think about it again.
- **Transport** (`@app/bridge/lib/transport/`) — Auto-detects QWebChannel vs WebSocket vs Embind. You never touch this.

## Three Transports, Same Code

| Mode | Transport | Bridge adapter | When |
|------|-----------|----------------|------|
| **Desktop prod** | QWebChannel (in-process) | `BridgeChannelAdapter` (QObject with `dispatch()`) | `xmake run desktop` |
| **Desktop dev/test** | WebSocket JSON-RPC | `expose_as_ws.hpp` | `xmake run dev-server`, Playwright, Bun tests |
| **Browser (WASM)** | Direct Embind calls | `WasmBridgeWrapper` (generic) | `xmake run dev-wasm` |

React auto-detects: `VITE_TRANSPORT=wasm` → Embind. `window.qt?.webChannelTransport` → QWebChannel. Otherwise → WebSocket to `localhost:9876`.

All three transports talk to the same `app_shell::Bridge` instance. The adapters convert between the bridge's native `nlohmann::json` and whichever transport's wire format applies (Qt JSON via `json_adapter.hpp`, raw JSON for WebSocket, `emscripten::val` for WASM).

This means you can develop in a browser with hot reload, the same code runs inside the Qt window in production, and the same C++ logic runs as WebAssembly in the browser — zero changes to React.

## The Proxy Pattern

Both sides are zero-boilerplate:

**C++ side:** The bridge base class handles everything. Register a method with `method("name", &Bridge::fn)` — template deduction figures out the request/response types. `def_type` reflection (via PFR) auto-serializes plain C++ structs to/from JSON. No hand-written `to_json()`, no `to_val()`, no type registration.

**TypeScript side:** `getBridge<TodoBridge>('todos')` queries the backend for available methods and signals via the `__meta__` protocol, then returns a JavaScript `Proxy`. Method calls become JSON-RPC messages. Signal names become subscribe functions.

**WASM side:** The generic `WasmBridgeWrapper` wraps any `app_shell::Bridge` for Embind. No per-bridge WASM code needed.

**The result:** Add a method to your domain logic, add a request DTO struct, register the method on the bridge, add a line to the TypeScript interface. The framework connects them.

## Bridge Calls Use Request Objects

Bridges take **one request object** per method, not positional arguments:

```typescript
// Correct
await todos.addItem({ list_id: id, text: 'Buy milk' })

// Wrong — won't work
await todos.addItem(id, 'Buy milk')
```

The C++ side mirrors this: each method takes a `Request` struct, and PFR walks the struct fields to deserialize. The request object pattern means adding a parameter is just adding a field — no positional-argument reshuffling across the wire.

## Signals — C++ to JavaScript Events

Bridges push real-time updates from C++ to React. Signals carry typed payload data:

```cpp
// C++ — register in the constructor
signal("itemAdded");

// Emit with a payload (any def_type struct works)
TodoItem addItem(AddItemRequest req) {
    auto item = store_.add_item(req.list_id, req.text);
    emit_signal("itemAdded", item);  // serialized via def_type::to_json
    return item;
}
```

```typescript
// TypeScript — subscribe with a typed callback
const cleanup = todos.itemAdded((item) => {
  console.log('item added:', item)
})
// Later: cleanup() to unsubscribe
```

Signals work across all three transports — WebSocket forwards them as JSON messages, QWebChannel re-emits them as Qt signals with JSON string payloads, WASM converts the JSON to a JS object before calling the callback.

## The signalReady() Contract (Desktop Only)

React calls `signalReady()` after mounting. This tells the C++ side to fade out the loading overlay. If it never fires (JS error, bridge broken), a 15-second timeout shows an error message.

This call must run after every app's mount — `App.tsx` in demo, settings, and app all invoke it. In WASM mode, `signalReady()` is a no-op (no loading overlay to dismiss).

## The Three Apps and Four Workspace Packages

The web layer is three Vite apps composing four workspace packages:

```
web/
  packages/
    ui/             ← @app/ui — shadcn primitives + use-sidebar-slot + cn
    theming/        ← @app/theming — themes, fonts, effects, panels, qt-sync
    monaco/         ← @app/monaco — editor wrapper + theme bridge
    bridge/         ← @app/bridge — transport + typed bridge declarations
  apps/
    demo/           ← the playground (every pattern lives here)
    settings/       ← thin app composing @app/theming
    app/            ← empty slate (react + react-router + @app/bridge)
  package.json      ← single deps, per-app scripts (build:demo, build:settings, build:app)
```

Each app has its own `vite.config.ts` and dev server port (5173 / 5174 / 5175). The Qt side routes between them using a custom URL scheme — `app://demo/`, `app://settings/`, `app://app/`. `App::appUrl("demo")` returns the right URL for dev or production.

Every app uses `HashRouter` from `react-router` at the top level. Custom URL schemes don't support `history.replaceState` with path changes (Chromium treats `app://` origins as scheme-only, so any path change is cross-origin), so hash routing is the only routing that works inside the Qt shell.

To add a new app, copy `web/apps/app/` (the smallest template), add it to `WEB_APPS` in `desktop/xmake.lua`, and add `build:<name>`/`dev:<name>` scripts to `web/package.json`. The [agent docs](../for-agents/03-adding-features.md) have the step-by-step recipe.

## Cross-Platform

The template builds on Windows, macOS, and Linux. Platform-specific bits:

- `app.rc` (Windows icon/version info) — auto-generated by xmake
- `gmtime_s` / `gmtime_r` — guarded by `#ifdef _MSC_VER`
- Native UI testing — pywinauto (Windows), atomacos (macOS), dogtail (Linux)

Everything else — React, Vite, Playwright, the bridge — is cross-platform by nature.
