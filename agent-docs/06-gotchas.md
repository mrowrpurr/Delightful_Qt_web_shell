# Gotchas — Everything That Will Trip You Up

## The Showstoppers

### cdp-mcp must run under Node, not Bun

Bun's `ws` polyfill can't handle HTTP 101 Switching Protocols, which CDP requires. If you run cdp-mcp with Bun, `connectOverCDP` hangs forever — no error, no timeout, just silence.

The `.mcp.json` uses `npx tsx`. Don't change it.

### signalReady() must not be removed

`web/src/App.tsx` calls `signalReady()` after React mounts. This tells the C++ side to fade out the loading overlay. If it never fires:

- User sees a spinner forever
- No error, no crash — just stuck
- 15-second timeout eventually shows an error message

If you refactor `App.tsx`, you can move the call. Just don't delete it.

### playwright-core needs patching for QtWebEngine

QtWebEngine doesn't support the `Browser.setDownloadBehavior` CDP command that Playwright sends during context initialization. Without the patch, `connectOverCDP` crashes.

The fix is a one-line `.catch(() => {})` in Playwright's `crBrowser.js`. It's applied automatically:
- Root `package.json` uses `patchedDependencies` (Bun's patch system)
- `tools/cdp-mcp/postinstall` applies the same patch to its local copy

**When bumping playwright-core versions**, check if the patch still applies cleanly and if the underlying issue has been fixed upstream.

## Build Gotchas

### Web build caching can fool you

The build skips Vite if `web/src/` hasn't changed (checked by file timestamps against `build/.web-build-stamp`). This is fast but can be confusing:

- **Symptom:** You edited web code but the app shows the old version
- **Fix:** Touch any file in `web/src/` to force a rebuild, or delete `build/.web-build-stamp`

### Qt MOC requires headers in add_files()

If you create a new bridge (a `QObject` subclass), its `.hpp` must be listed in `add_files()` in **both**:
- `desktop/xmake.lua`
- `tests/helpers/dev-server/xmake.lua`

If you skip this, you get a cryptic vtable linker error that doesn't mention your file at all. It's always the MOC.

### Bridge registration in two places

New bridges must be registered in both entry points:
- `desktop/src/main.cpp` → `shell->addBridge("name", bridge)`
- `tests/helpers/dev-server/src/test_server.cpp` → `shell.addBridge("name", bridge)`

If you only register in `main.cpp`, browser-mode dev and Playwright tests won't see your bridge. No error — it silently doesn't exist, and you'll waste time debugging React thinking the method call is wrong.

### First build is slow

`xmake build desktop` takes ~30s on first run (Vite + C++ compile). Subsequent builds skip Vite if web source hasn't changed and only recompile changed C++ files.

## Test Gotchas

### Desktop tests need the app built first

```bash
xmake build desktop    # must do this first
xmake run test-desktop
```

If you skip the build, the test runner can't find the executable.

### Desktop tests are inherently flaky

GPU rendering, window manager timing, and focus issues make desktop tests less stable than browser tests. If a desktop test fails but the browser version of the same test passes, it's probably a timing/rendering issue, not a code bug.

### The app must be running for pywinauto

```bash
xmake run start-desktop    # start it first
xmake run test-pywinauto   # then test
```

pywinauto connects to a running window. If the app isn't running, every test fails with "window not found."

### Port conflicts

| Port | Used by | If busy |
|------|---------|---------|
| 5173 | Vite dev server | Another Vite instance running? |
| 9222 | CDP (Qt debug port) | Another Qt instance or Chrome with debug? |
| 9876 | WebSocket bridge (dev/test) | dev-server already running? |

### Debug bottom-up

If tests fail, start from the bottom:
1. Catch2 fails → C++ logic is broken
2. Catch2 passes, Bun fails → bridge protocol is broken
3. Bun passes, browser e2e fails → UI isn't wired up correctly
4. Browser passes, desktop fails → GPU/window issue (not your code)

## Type System

### Types just work — don't overthink it

The bridge uses Qt's `QVariant` conversion system. Any type Qt can convert to/from JSON works automatically. You never need to modify the framework to support a new type.

Common types: `QString`, `int`, `double`, `bool`, `QJsonObject`, `QJsonArray`, `QStringList`, `QVariant`. But if Qt can serialize it, it works.

### Max 10 parameters per method

Qt's `QMetaObject::invokeMethod` supports up to 10 arguments. If you need more, pass a `QJsonObject` instead.

### Void methods return `{ok: true}`

Not `undefined`, not `null` — the JS side gets `{ok: true}`. Your TypeScript interface should return `Promise<{ok: true}>` or just `Promise<void>` (the proxy handles it).

### Arg count mismatch gives a clear error

```
"addItem: expected 2 args, got 1"
```

No silent nulls, no crash — you get an error message saying exactly what's wrong.

## Environment

### .env.example exists — check it

`.env.example` documents the environment variables Vite uses. Copy it to `.env` for local overrides. The key one is `VITE_BRIDGE_WS_URL` (defaults to `ws://localhost:9876`).

### Windows is the primary platform

pywinauto is Windows-only. cdp-mcp works everywhere. The full test suite (all 5 layers) only runs on Windows. macOS/Linux can run everything except pywinauto tests.

## The Validate Command

```bash
xmake run validate-bridges
```

This checks that your TypeScript interfaces match your C++ `Q_INVOKABLE` methods. Run it after adding or changing bridge methods. It catches drift at dev time — before you find out at runtime that a method name is misspelled or an argument is missing.
