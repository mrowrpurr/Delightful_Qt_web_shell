# Pywinauto Verification — Handoff 🏴‍☠️

A test trim just landed. Two pywinauto test files were removed, two were kept. Your job: verify the surviving tests run green on Windows.

## What happened

- **Removed:** `app/tests/pywinauto/test_menu_bar.py`, `app/tests/pywinauto/test_window.py`
- **Kept:** `app/tests/pywinauto/test_full_dialog_flow.py`, `app/tests/pywinauto/test_keyboard_shortcuts.py`
- **Helpers untouched:** `conftest.py`, `native_dialogs.py`, `win32_helpers.py`

## What to do

```bash
# 1. Build the desktop app
xmake build desktop

# 2. Launch it in background with CDP
xmake run start-desktop

# 3. Run pywinauto
xmake run test-pywinauto

# 4. Stop the app
xmake run stop-desktop
```

## What "green" means

- `test_full_dialog_flow.py` passes — this exercises menu → modal → file dialog → Win32 API driving
- `test_keyboard_shortcuts.py` passes — this exercises set_focus → type_keys → verify result

## If something fails

- If a test fails because of a **changed menu name or shortcut**, that's a real regression from the native refactor (Phases 1–2 extracted subsystems and renamed things). Fix the test to match current menu/shortcut names.
- If a test fails because of a **timing issue** (dialog didn't appear in time), bump the `time.sleep()` — Qt dialogs on Windows can be slow to paint.
- If `conftest.py` can't find the app window, check that `xmake run start-desktop` actually launched and the window title matches what conftest expects.

## Context

- The app window title is controlled by `APP_NAME` in `xmake.lua` (currently "Delightful Qt Web Shell")
- `test_full_dialog_flow.py` uses `native_dialogs.py` helpers (`FileDialog`, `QtMessageBox`, `open_modal`) which use Win32 API to bypass Qt's modal event loop blocking UIA
- `test_keyboard_shortcuts.py` uses pywinauto's `type_keys` directly — no modal complexity

## Do NOT

- Do not add tests back. The trim was intentional.
- Do not run destructive git commands.
- Do not modify the helpers unless a test needs it to pass.
