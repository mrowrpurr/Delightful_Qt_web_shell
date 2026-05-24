# Gotchas

Things that will bite you if you don't know about them.

## Silent Failures

| What you forgot | What happens |
|---|---|
| Register bridge in `main.cpp` (in your app's entry point, e.g. `apps/demo/src/main.cpp`) and `test_server.cpp` | Bridge silently doesn't exist — no error, just missing |
| Bridge call with positional args | Bridges take a **request object** (`addItem({list_id, text})`), not positional args |
| Remove `signalReady()` from `App.tsx` | App hangs with spinner forever, error after 15s |
| Use Bun instead of Node for playwright-cdp | `connectOverCDP` hangs forever — no error, no timeout |
| playwright-cdp fails with `ERR_MODULE_NOT_FOUND` | Deps not installed in `tools/playwright-cdp/` — it's a separate install from the Bun workspace. Run `cd tools/playwright-cdp && npm install` (or `xmake run app.setup`). |
| playwright-cdp fails with `connectOverCDP: Timeout 30000ms exceeded` | The desktop app's CDP endpoint is stuck. Restart the app: `xmake run app.demo.stop && xmake run app.demo.start`. |
| Bridge method opens modal dialog synchronously | Dialog's QWebChannel can't init — loading overlay forever |
| Drag & drop handler on WebShellWidget | QWebEngineView's focusProxy swallows all drag events |
| Native `<select>` in QWebEngine | White rectangle appears — use custom dropdown component |
| `fetch()` with `app://` scheme | Doesn't work — use Vite JSON import at build time |

**Tip:** Use `xmake run app.bridge.scaffold <name>` to create new bridges. It handles registration automatically.

## Build

- **Every build runs Vite** (~30s) then compiles C++ (~10s).
- **Skip Vite for C++ iteration:** `SKIP_VITE=1 xmake build app.demo` reuses the previous web bundle (~2s). Works with `run app.demo` and `run app.demo.start` too.
- **`xmake build app.demo` before desktop tests.** The app binary must exist.

## Multi-App / Vite

- **`vite --config` doesn't change root.** Use `cd apps/demo && vite build` in scripts, not `vite build --config apps/demo/vite.config.ts`.
- **Vite inlines assets < 4KB as data URIs.** Set `assetsInlineLimit: 0` in every app's `vite.config.ts` — QWebEngine can't always handle them.
- **Workspace package resolution from outside `web/`.** Bun hoists `@app/*` symlinks under `web/node_modules/@app/`. Code outside `web/` (e.g., bun tests under `framework/qt-transport/tests/web/`) needs the matching dep declared at `app/package.json` so a top-level `app/node_modules/@app/<name>` symlink exists.

## Theming

- **`fetch` doesn't work with `app://`** — import JSON at build time via Vite.
- **Theme vars need both `--background` and `--color-background`** for Tailwind v4 compatibility.
- **QSS doesn't support CSS tricks** like border-triangles for dropdown arrows — use SVG images.
- **`QIODevice::Text` doubles `\r\n` on Windows** — use binary mode for `writeTextFile`.

## WASM

- **`set_kind("object")` not `"static"`** for the WASM bridges library — static gets dead-stripped.
- **Can't `import()` from `/public` in Vite** — use blob URL + `<script type="module">`. See `wasm-transport.ts`.
- **Platform switch resets Qt path** — always pass `--qt=` when switching back from WASM.

## Ports

| Port | Used by |
|------|---------|
| 5173 | Vite dev server |
| 9222 | CDP (Qt debug port) |
| 9333 | CDP (playwright-cdp persistent browser) |
| 9876 | WebSocket bridge (dev-server) |

## Other

- **`QCommandLineParser.parse()` not `process()`** — `process()` shows error dialogs on unknown flags.
- **playwright-core patch** — QtWebEngine doesn't support `Browser.setDownloadBehavior`. Applied automatically by `patchedDependencies`.
- **pywinauto is Windows-only.** macOS/Linux can run everything except pywinauto tests.
