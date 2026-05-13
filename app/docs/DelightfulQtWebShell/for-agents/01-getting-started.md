# Getting Started

You're an agent who wants to build an app. This template gives you Qt + React + C++ with a bridge between them, four layers of automated testing, and two deployment targets: **desktop** (Qt) and **browser** (WASM).

## What You Get

- **React UI** — one codebase, two targets: native Qt desktop window or standalone browser app
- **C++ backend** connected to the UI via bridges — one pure C++ bridge class serves both desktop and WASM
- **Shared domain logic** — pure C++ with no framework deps, compiled for both targets
- **Four test layers** that actually work — C++ unit, bridge protocol, browser e2e, native Qt
- **Dev tools** — playwright-cdp (see/click web content via CLI), pywinauto (drive native Qt widgets)

## Project Layout

Two roots: `<repo>/lib/` for portable pure C++ that has nothing to do with this template (consumer can lift it to other projects), and `<repo>/app/` for everything the template owns.

```
<repo>/
├── lib/                              # Portable pure C++ — no Qt, no Emscripten, no app_shell::
│   └── todos/                        #   Example domain (consumer replaces or deletes)
│       ├── include/
│       │   ├── todo_store.hpp        #     Domain logic — pure C++ structs + operations
│       │   └── todo_dtos.hpp         #     Request DTOs for TodoBridge methods
│       └── tests/unit/               #     Catch2 tests live with the domain
└── app/                              # The template itself
    ├── framework/                    # Bridge base + transports + lifecycle (don't touch)
    │   ├── bridge/                   #   app_shell::Bridge base
    │   ├── bridge-registry/          #   Pure-C++ registry of bridges by name
    │   ├── app-lifecycle/            #   Qt QObject — appReady signal
    │   ├── qt-transport/             #   QWebChannel adapter + WebSocket server
    │   └── wasm-transport/           #   Embind wrapper (header-only)
    ├── bridges/                      # Bridge classes — wrap a domain area, extend app_shell::Bridge
    │   ├── todos/include/            #   TodoBridge — wraps <repo>/lib/todos
    │   └── system/                   #   SystemBridge — Qt-dependent file/clipboard/dialogs
    ├── desktop/src/                  # Qt desktop binary
    │   ├── main.cpp                  #   Entry point — scheme registration, app, window, show
    │   ├── shell/                    #   App — QApplication, registry, lifecycle, tray
    │   ├── windows/main_window.*     #   QMainWindow — wires menus, toolbar, status bar, web view
    │   ├── menus/menu_bar.*          #   Menu bar + toolbar construction
    │   ├── widgets/
    │   │   ├── web_shell_widget.*    #     QWidget wrapping QWebEngineView + bridges + overlay
    │   │   ├── loading_overlay.*     #     Loading overlay (Full or Spinner mode)
    │   │   ├── scheme_handler.*      #     app:// URL scheme for embedded resources
    │   │   └── status_bar.*          #     Status bar (zoom %, status, flash messages)
    │   └── dialogs/
    │       ├── about_dialog.*        #     Custom QDialog example
    │       └── web_dialog.*          #     React-in-a-dialog (WebShellWidget in a QDialog!)
    ├── wasm/                         # WASM entry point + Emscripten linker config
    ├── web/                          # React workspaces (Vite) — shared by desktop + WASM
    │   ├── packages/                 #   Workspace packages
    │   │   ├── ui/                   #     @app/ui — shadcn primitives + useSidebarSlot + cn
    │   │   ├── theming/              #     @app/theming — themes, fonts, effects, panels, qt-sync
    │   │   ├── monaco/               #     @app/monaco — editor wrapper + theme bridge
    │   │   └── bridge/               #     @app/bridge — transport + typed bridge declarations
    │   │       └── lib/
    │   │           ├── transport/    #       Framework runtime (don't touch)
    │   │           └── bridges/      #       Typed bridge declarations (add yours here)
    │   ├── apps/                     #   Three Vite apps
    │   │   ├── demo/                 #     Playground — every pattern lives here
    │   │   ├── settings/             #     Thin app composing @app/theming
    │   │   └── app/                  #     Empty slate — react + react-router + @app/bridge
    │   └── package.json              #   Per-app scripts (build:demo, build:settings, build:app)
    ├── tests/
    │   ├── playwright/               #   Browser + desktop e2e tests
    │   ├── pywinauto/                #   Native Qt widget tests (Windows)
    │   └── helpers/dev-server/       #   Headless C++ backend for dev/test
    ├── tools/playwright-cdp/         # Playwright CLI for driving web content
    └── xmake/                        # xmake target definitions (dev, dev-wasm, scaffolding, etc.)
```

**dev-server** is a headless C++ process that serves your bridges over WebSocket on port 9876 — no Qt window, no GUI. It's what runs during `xmake run dev-server`, Playwright browser tests, and Bun tests. Same bridge code as the desktop app, just without a window.

## Prerequisites

- [xmake](https://xmake.io) — build system
- [Qt 6.x](https://www.qt.io) with modules: WebEngine, WebChannel, WebSockets, Positioning (Positioning is a transitive dependency of QtWebEngine — you won't use it directly)
- [Bun](https://bun.sh) — JS runtime and package manager
- [Node.js](https://nodejs.org) — for Playwright and playwright-cdp (Bun's ws polyfill breaks CDP)
- [Emscripten](https://emscripten.org) — *(optional, WASM target only)* C++ to WebAssembly compiler

## Make It Yours

Edit the top of `xmake.lua`:

```lua
APP_NAME    = "Your App Name"
APP_SLUG    = "your-app-name"
APP_ORG     = "YourOrganization"
APP_VERSION = "0.1.0"
```

This flows everywhere: window title, binary name, Windows exe metadata, HTML `<title>`, loading screen, and platform settings/data directories (`QSettings`, `AppLocalDataLocation`). Replace `desktop/resources/icon.ico` and `icon.png` with your own.

## First-Time Setup

```bash
# Point xmake at your Qt installation
xmake f --qt=/path/to/qt   # e.g. C:/Qt/6.10.2/msvc2022_64

# Windows example:
# xmake f -m release -p windows -a x64 --qt="C:/qt/6.10.2/msvc2022_64" -c -y

# Install all dependencies (uv, bun, playwright-cdp, playwright chromium)
xmake run setup
```

## Build & Run

```bash
# Build the desktop app (builds React via Vite, then C++)
xmake build desktop

# Run it
xmake run desktop
```

Every build runs Vite (~30s) then compiles C++ (~10s). Use `SKIP_VITE=1` below when you're only changing C++.

### Skip Vite (C++ iteration)

When you're only changing C++, skip the entire Vite build with `SKIP_VITE=1`:

```bash
SKIP_VITE=1 xmake build desktop       # ~2s instead of ~40s
SKIP_VITE=1 xmake run desktop         # build + run, no Vite
SKIP_VITE=1 xmake run start-desktop   # background launch, no Vite
```

Requires a previous Vite build — if `web_dist_resources.cpp` doesn't exist, it warns and builds anyway. This skips `bun install`, both Vite builds, qrc generation, and rcc.

## Dev Mode

Two workflows depending on what you're working on:

### React + C++ together (HMR inside Qt)

```bash
# Terminal 1: Vite dev server with hot reload
xmake run dev-web

# Terminal 2: Qt app loading from Vite + CDP on :9222
xmake run dev-desktop
```

Edit React components, save, see changes instantly inside the native Qt window.

**React changes are live. C++ changes require rebuild + restart:** `xmake build desktop && xmake run dev-desktop`. There's no C++ hot reload — plan your workflow accordingly.

### React only (no Qt needed)

```bash
# Terminal 1: C++ backend over WebSocket
xmake run dev-server

# Terminal 2: Vite dev server
xmake run dev-web

# Open http://localhost:5173 in any browser
```

Same React code, same bridge calls — just running in a browser instead of Qt.

### WASM (browser-only, no Qt needed)

```bash
# One-time: build the WASM target
xmake f -p wasm && xmake build wasm-app

# Switch back to desktop config (WASM artifacts persist in build/)
xmake f -p windows --qt=/path/to/qt

# Run the WASM app in browser
xmake run dev-wasm
```

`dev-wasm` copies the WASM build artifacts to `web/public/` and starts Vite with `VITE_TRANSPORT=wasm`. Same React UI, same method names — but the C++ runs as WebAssembly in the browser instead of a Qt backend.

**React HMR works.** C++ changes require `xmake f -p wasm && xmake build wasm-app`, then `reload()` in playwright-cdp or refresh the browser.

**Drive it with playwright-cdp:**
```bash
# Headless (agent solo)
PLAYWRIGHT_URL=http://localhost:5173 npx tsx tools/playwright-cdp/cli.ts snapshot

# Headed (pairing with human — browser stays open between commands)
npx tsx tools/playwright-cdp/cli.ts open http://localhost:5173
PLAYWRIGHT_URL=http://localhost:5173 npx tsx tools/playwright-cdp/cli.ts snapshot
npx tsx tools/playwright-cdp/cli.ts close
```

### Storybook (component library)

```bash
xmake run storybook   # opens on http://localhost:6006
```

Browse and test shared UI components (Button, Card, Select, Tabs) in isolation. No backend needed — no Qt, no WASM, no WebSocket.

**Custom panels** — the **Theme** panel (bottom, next to Controls/Actions) has:
- **Theme picker** — searchable list of 1000+ shadcn themes with color preview dots
- **Dark/Light toggle** — switches all components instantly
- **Font picker** — searchable list of 1900+ Google Fonts with category labels

Themes and fonts are the same system the app uses (`@app/theming/lib/themes.ts`, `@app/theming/lib/fonts.ts`). Selection persists in localStorage across Storybook sessions.

**Story files** live in `web/packages/ui/components/*.stories.tsx`. To add a story for a new component, create a `.stories.tsx` file next to it — Storybook scans `packages/ui/components/**/*.stories.@(ts|tsx)`.

**Key files:**
- `.storybook/main.ts` — Vite config, addons, story glob
- `.storybook/preview.ts` — global CSS, theme/font initialization, addon channel listeners
- `.storybook/manager.tsx` — theme/font addon panel UI

### Background launch (for automation)

```bash
xmake run start-desktop    # launches app in background, CDP on :9222
xmake run stop-desktop     # kills it
```

Check if it's running:
```bash
curl -s http://localhost:9222/json/version
```

## Quick Test

```bash
xmake run test-all   # Catch2 + Bun + Playwright + pywinauto
```

Runs all layers except desktop e2e (Playwright in Qt). Launches and stops the desktop app automatically for pywinauto tests.

If that's green, everything works. See [Testing](04-testing.md) for the full picture.
