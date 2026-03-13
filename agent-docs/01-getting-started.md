# Getting Started

You're an agent who wants to build a desktop app. This template gives you Qt + React + C++ with a bridge between them and five layers of automated testing.

## What You Get

- **React UI** rendered inside a Qt WebEngine window — you write React, the user sees a native desktop app
- **C++ backend** connected to the UI via a type-safe bridge — write `Q_INVOKABLE` methods, call them from TypeScript
- **Five test layers** that actually work — C++ unit, bridge protocol, browser e2e, desktop e2e, native Qt
- **Dev tools** — cdp-mcp (see/click web content via MCP), pywinauto (drive native Qt widgets)

## Prerequisites

- [xmake](https://xmake.io) — build system
- [Qt 6.x](https://www.qt.io) with modules: WebEngine, WebChannel, WebSockets, Positioning
- [Bun](https://bun.sh) — JS runtime and package manager
- [Node.js](https://nodejs.org) — for Playwright and cdp-mcp (Bun's ws polyfill breaks CDP)

## Make It Yours

Edit the top of `xmake.lua`:

```lua
APP_NAME    = "Your App Name"
APP_SLUG    = "your-app-name"
APP_VERSION = "0.1.0"
```

This flows everywhere: window title, binary name, Windows exe metadata, HTML `<title>`, loading screen. Replace `desktop/resources/icon.ico` and `icon.png` with your own.

## Build & Run

```bash
# Point xmake at your Qt installation (one time)
xmake f --qt=/path/to/qt   # e.g. C:/Qt/6.10.2/msvc2022_64

# Build the desktop app (builds React via Vite, then C++)
xmake build desktop

# Run it
xmake run desktop
```

The first build takes ~30s (Vite + C++ compile). Subsequent builds skip Vite if `web/src/` hasn't changed.

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

### React only (no Qt needed)

```bash
# Terminal 1: C++ backend over WebSocket
xmake run dev-server

# Terminal 2: Vite dev server
xmake run dev-web

# Open http://localhost:5173 in any browser
```

Same React code, same bridge calls — just running in a browser instead of Qt.

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
xmake run test-all   # Catch2 + Bun + Playwright browser (~10s)
```

If that's green, everything works. See [Testing](04-testing.md) for the full picture.
