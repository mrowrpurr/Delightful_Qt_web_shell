# Session 3 Handoff (2026-03-20)

## What Was Done This Session

### 1. Multi-Web-App Architecture (HUGE change)
Restructured `web/` from single Vite app to N apps sharing code:

```
web/
  package.json          ← single deps, per-app scripts (build:main, build:docs, dev:main, dev:docs)
  shared/api/           ← bridge transport + TS interfaces (moved from web/src/api/)
    bridge.ts, bridge-transport.ts, system-bridge.ts, wasm-transport.ts
  apps/
    main/               ← todo app (moved from web/src/)
      vite.config.ts    ← has @shared alias → ../../shared, port 5173
      tsconfig.app.json ← paths: @shared/*, includes shared dir
      src/App.tsx, DialogView.tsx, main.tsx, App.css
    docs/               ← NEW second app — welcome/architecture docs page, port 5174
```

**Key decisions:**
- Single package.json at web/ (shared node_modules, one bun install)
- Scripts use `cd apps/<name> && vite build` (Vite needs CWD = config dir, --config flag does NOT change root)
- `@shared` Vite alias + tsconfig paths — no workspace packages needed
- Each app's tsconfig.app.json includes `["src", "../../shared"]`

**C++ changes:**
- **SchemeHandler** routes by host: `app://main/` → `:/web-main/`, `app://docs/` → `:/web-docs/`
- **WebShellWidget** takes `QUrl contentUrl` instead of `bool devMode`
- **Application::appUrl(appName)** returns right URL for dev/prod. Dev ports: main=5173, docs=5174
- **MainWindow** uses QSplitter: two WebShellWidgets (main 2/3, docs 1/3)
- **desktop/xmake.lua** has `WEB_APPS = {"main", "docs"}` list, builds each, combined qrc with `/web-main` and `/web-docs` prefixes

**Build system:**
- `xmake/dev.lua` — `dev-web-main`, `dev-web-docs` targets
- `xmake/dev-wasm.lua` — paths updated to web/apps/main/public/
- `xmake/scaffold-bridge.lua` — TS stubs go to web/shared/api/
- Test imports updated: `lib/web-shell/tests/web/*.ts` → `web/shared/api/`

### 2. Qlementine Icons
- User added `qlementine-icons` via SkyrimScriptingBeta xmake repo in root xmake.lua
- `desktop/xmake.lua` gets `add_packages("qlementine-icons")`
- `application.cpp` calls `oclero::qlementine::icons::initializeIconTheme()` at startup
- `menu_bar.cpp` has `tintedIcon()` helper — loads SVG, paints white via `QPainter::CompositionMode_SourceIn`
- Icons: Action_Save, File_FolderOpen, Action_ZoomIn/ZoomOut/ZoomOriginal, Navigation_Settings
- **Gotcha:** No Action_Open (use File_FolderOpen), no ZoomReset (use ZoomOriginal), no Action_Settings (use Navigation_Settings)

### 3. React Hash Routing for Dialogs
- `main.tsx` checks `window.location.hash` — `#/dialog` → DialogView, else → App
- `DialogView.tsx` — "Quick Add Todo" with list dropdown + item input, shares bridges
- `web_dialog.cpp` appends `#/dialog` via `QUrl::setFragment("/dialog")`
- Extensible: `#/settings`, `#/about`, etc.

### 4. React → Qt Dialog Opening (bridge call)
- `SystemBridge::openDialog()` — Q_INVOKABLE emits `openDialogRequested()` signal
- `main_window.cpp` connects signal → opens WebDialog
- React has "🗂️ Quick Add" button calling `system.openDialog()`
- `system-bridge.ts` updated with openDialog() + openDialogRequested signal
- `main_window.cpp` includes `web_shell.hpp`, `system_bridge.hpp`, `dialogs/web_dialog.hpp`

## Git State
- Branch: `qt-delightfulness`
- Committed: `969abc7 ✨ qlementine-icons for menus & toolbar` and `3422a2e multiple web apps!`
- **UNCOMMITTED**: dialog routing (DialogView.tsx, main.tsx hash check, web_dialog.cpp fragment, openDialog bridge, Quick Add button)
- Everything compiles and links clean

## What's NOT Done
- Agent docs still reference old `web/src/` paths
- WASM bridge doesn't have openDialog (desktop-only, fine)
- User wants: dark/light theme toggle (QSS), drag & drop demo, single-instance file path passing
