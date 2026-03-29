# Tools

Dev tools for debugging and testing your app.

## DevTools (F12)

The web content runs in a Chromium-based engine. Press **F12** or use **Windows > Developer Tools** to open the Chrome DevTools inspector — same as a browser, with console, network, elements, everything.

## pywinauto — Automated Testing of Native Qt

Python library for testing native Windows UI — menus, dialogs, keyboard shortcuts. Useful for verifying that your Qt widgets work correctly without manual clicking.

```bash
xmake run start-desktop              # launch the app
uv run pytest tests/pywinauto/ -v    # run the tests
```

Tests live in `tests/pywinauto/`. They drive the app's menus, open dialogs, and verify window behavior.

**Heads up:** Qt6 modal dialogs (QMessageBox, QFileDialog) block pywinauto's UIA backend. Use the Win32 API helpers in `tests/pywinauto/native_dialogs.py` to work around this.

## Desktop Screenshots

Capture the full screen from a script (useful for CI or debugging native dialogs):

```bash
uv run python tools/screenshot.py                 # primary monitor
uv run python tools/screenshot.py -o debug.png     # custom path
```

## Platform Support

| Tool | Windows | macOS | Linux |
|------|---------|-------|-------|
| DevTools (F12) | ✅ | ✅ | ✅ |
| pywinauto | ✅ | ❌ | ❌ |
| Screenshots | ✅ | ✅ | ✅ |
