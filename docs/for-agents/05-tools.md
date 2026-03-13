# Tools — Seeing and Driving the App

You're an agent. You can't look at a screen. These tools are your eyes and hands.

## Two Tools, Two Layers

```
 cdp-mcp (MCP)              pywinauto (Python)
 ┌──────────────┐           ┌──────────────┐
 │ Web content  │           │ Native Qt    │
 │ React DOM    │           │ Menus        │
 │ CSS layout   │           │ Dialogs      │
 │ Console logs │           │ Shortcuts    │
 └──────────────┘           └──────────────┘
      ▲                          ▲
      │                          │
  CDP on :9222              UIA automation
  (Chrome DevTools)         (Windows only)
```

| Tool | What it sees | What it drives | When to use |
|------|-------------|----------------|-------------|
| **cdp-mcp** | Web content rendered by React | Click, fill, evaluate JS | Anything inside the web view |
| **pywinauto** | Native Qt widgets | Menus, dialogs, keyboard shortcuts | Anything outside the web view |

**Rule of thumb:** If a human would right-click or use a menu → pywinauto. If they'd click a button in the UI → cdp-mcp.

## cdp-mcp — Your Eyes on the Web Content

An MCP server that connects to the Qt app's Chrome DevTools Protocol endpoint.

### Setup

The app must be running with CDP enabled:

```bash
xmake run start-desktop    # launches app, CDP on :9222
```

Verify CDP is up:
```bash
curl -s http://localhost:9222/json/version
```

### Available MCP Tools

| Tool | What it does |
|------|-------------|
| `snapshot` | Returns the accessibility tree (DOM structure with roles, names, values) |
| `screenshot` | Takes a PNG screenshot of the page |
| `click` | Clicks an element by accessibility snapshot ref |
| `fill` | Types text into an input field |
| `press` | Sends a key press (Enter, Tab, etc.) |
| `evaluate` | Runs arbitrary JavaScript in the page context |
| `text_content` | Gets the text content of the page |
| `wait_for` | Waits for text to appear on the page |
| `console_messages` | Returns recent console.log output |

### Workflow Pattern

1. **Orient** — take a `snapshot` to see the current DOM state
2. **Act** — `click`, `fill`, or `press` to interact
3. **Verify** — `snapshot` again or `text_content` to confirm the result
4. **Debug** — `console_messages` if something went wrong, `evaluate` for deeper inspection

### Example: Add a Todo Item

```
→ snapshot()                          # see the current UI state
→ click(ref: 5)                       # click the input field (ref from snapshot)
→ fill(ref: 5, value: "Buy milk")     # type into it
→ click(ref: 7)                       # click the "Add" button
→ snapshot()                          # verify the item appeared
```

### Tips

- **snapshot is your primary tool** — it's fast, gives you the accessibility tree with refs you can click
- **screenshot when snapshot isn't enough** — layout issues, visual bugs, "is this actually rendering?"
- **evaluate for power moves** — read React state, check localStorage, trigger functions
- **console_messages for debugging** — bridge errors, WebSocket issues, JS exceptions all show up here

### ⚠️ Critical: Node, Not Bun

cdp-mcp **must** run under Node.js (`npx tsx`), not Bun. Bun's WebSocket polyfill breaks the CDP connection — it can't handle the HTTP 101 Switching Protocols upgrade. The `.mcp.json` config already uses `npx tsx`. Don't change it to `bun run`.

## pywinauto — Your Hands on Native Qt

Python library that drives Windows UI Automation (UIA) to interact with native Qt widgets — the things React can't see.

### Setup

```bash
pip install pywinauto assertpy
# or
uv pip install pywinauto assertpy
```

The app must be running:
```bash
xmake run start-desktop
```

### Quick Start

```python
from pywinauto import Desktop

desktop = Desktop(backend="uia")
app = desktop.window(title="Delightful Qt Web Shell", class_name="QMainWindow")
app.wait("visible", timeout=5)
```

### ⚠️ Critical: Qt6 Modal Dialogs Block UIA

When a Qt6 modal dialog (QMessageBox, QFileDialog) is open, **pywinauto's UIA
backend is completely blocked**. `Desktop.windows()`, `child_window()`, `.click()`
— all hang forever. This is because Qt's modal event loop blocks the UIA COM server.

**Solution:** Use the Win32 API helpers in `tests/pywinauto/native_dialogs.py` and
`tests/pywinauto/win32_helpers.py`. The Win32 API (`EnumWindows`, `SendMessage`,
`PostMessage`) is unaffected by Qt's modal loop.

### Common Patterns

**Open a menu that triggers a modal dialog:**
```python
from native_dialogs import open_modal, QtMessageBox, FileDialog

# open_modal runs menu_select in a thread to avoid blocking
open_modal(app, "Help->About")
time.sleep(0.5)
```

**Drive a QMessageBox (About dialog):**
```python
# QMessageBox has NO Win32 child controls — Qt draws buttons itself.
# Use keyboard: Enter → default button (OK), Escape → cancel.
with QtMessageBox("About") as dlg:
    assert dlg.is_open
    dlg.press_ok()       # PostMessage VK_RETURN
```

**Drive a native file dialog (Export/Save/Open):**
```python
# Native Windows file dialog (#32770) has real Win32 child controls.
with FileDialog("Export Data") as dlg:
    dlg.set_filename("my_data.json")    # WM_SETTEXT on Edit
    dlg.navigate("C:/Users/Desktop")    # type path + Enter
    print(dlg.current_folder)           # read from address bar toolbar
    print(dlg.file_types)               # ['JSON Files (*.json)', 'All Files (*)']
    dlg.select_file_type(1)             # switch filter
    dlg.cancel()                        # BM_CLICK on Cancel button
```

**Non-modal interactions (menus, keyboard shortcuts, window props) still use pywinauto:**
```python
app.menu_select("File")     # open menu (non-modal — fine)
app.type_keys("{ESC}")       # close menu
app.set_focus()
app.type_keys("{F12}")       # F12 (DevTools is non-modal — fine)
assert app.is_visible()
```

### Writing pywinauto Tests

Tests live in `tests/pywinauto/`. Use the shared fixtures from `conftest.py`:

```python
# tests/pywinauto/test_my_feature.py
import time
from native_dialogs import FileDialog, open_modal

def test_my_dialog(app):
    open_modal(app, "File->My New Feature")
    time.sleep(1)

    with FileDialog("My Feature") as dlg:
        dlg.set_filename("output.json")
        dlg.save()
```

The `close_dialogs` autouse fixture sends `WM_CLOSE` to known dialog titles after
each test, preventing cascading failures.

Run tests:
```bash
xmake run start-desktop && xmake run test-pywinauto
# or directly:
uv run pytest tests/pywinauto/ -v
```

### Tips

- **Always use `open_modal()` for menu items that open modal dialogs** — `menu_select` blocks forever otherwise
- **Use `threading.Thread(daemon=True)` for keyboard shortcuts that open modals** (e.g., Ctrl+E)
- **Always `set_focus()` before keyboard shortcuts** — the app must be focused
- **Add `time.sleep(0.5-1)` after opening dialogs** — they take a moment to appear
- **`close_dialogs` fixture prevents test pollution** — runs automatically via `autouse=True`
- **`FileDialog` context manager auto-closes** on exit if still open (safety net)

## Desktop Screenshots — Your Eyes on the Full Screen

Agents can't see the screen. CDP `screenshot` only captures web content inside the
WebEngine. For native dialogs, menus, taskbar, error popups — anything outside the
web view — use the desktop screenshot tool.

### CLI (from any agent)

```bash
uv run python tools/screenshot.py                    # primary monitor → screenshot.png
uv run python tools/screenshot.py --monitor 2        # specific monitor
uv run python tools/screenshot.py --all              # all monitors as one image
uv run python tools/screenshot.py -o debug.png       # custom output path
uv run python tools/screenshot.py --list             # list available monitors
```

The output path is printed to stdout. Read the file to see the image.

### From Python tests (pywinauto)

```python
from screenshot import capture

path = capture()                                     # primary monitor → screenshot.png
path = capture(output="debug.png")                   # custom path
path = capture(monitor_index=2, output="mon2.png")   # specific monitor
path = capture(capture_all=True)                     # all monitors
```

No subprocess needed — `tools/` is on the Python path.

### From Playwright / Bun tests

```typescript
import { execSync } from 'child_process'
execSync('uv run python tools/screenshot.py -o test-results/desktop.png')
```

### Privacy

Disabled in CI by default (screenshots may capture sensitive content). Set
`SCREENSHOTS_ENABLED=1` to enable in CI environments.

## Cross-Layer Testing

Sometimes you need both tools together. Example: test that a React button triggers a native dialog.

```
1. cdp-mcp: click the "Export" button in React
2. pywinauto: verify the native QFileDialog appeared
3. pywinauto: click "Cancel" to close it
4. cdp-mcp: verify the UI shows "Export cancelled"
```

This is the superpower — React can't see native dialogs, pywinauto can't see React DOM. Together they cover everything.

## Platform Notes

| Platform | cdp-mcp | pywinauto | Notes |
|----------|---------|-----------|-------|
| **Windows** | ✅ | ✅ | Full support. Primary dev platform. |
| **macOS** | ✅ | ❌ (use atomacos) | pywinauto is Windows-only. atomacos is the macOS equivalent but less mature. |
| **Linux** | ✅ | ❌ (use dogtail) | dogtail or AT-SPI2 for native widget automation. |

cdp-mcp works everywhere because it talks to the browser engine, not the OS. Native widget testing is platform-specific.

## Sharing the Desktop with Your Human

If you're driving the app with pywinauto, **your human can't use their desktop** — you're moving their mouse, opening dialogs, pressing keys. Coordinate:

- Ask before taking over the desktop
- Work in focused bursts — do your automation, then release
- Consider running on a separate machine or VM for long automation sessions
- cdp-mcp doesn't have this problem — it works through CDP, invisible to the human
