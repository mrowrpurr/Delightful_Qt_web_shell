# Win32 Dialog Exploration Notes

## Root Cause Discovery

**Qt6 modal dialogs block the UIA COM server.** When a modal QMessageBox or
QFileDialog is open, pywinauto's UIA backend cannot enumerate, find, or interact
with ANY window — not just the dialog, but the entire app. Even `Desktop.windows()`
hangs forever.

The Win32 API (EnumWindows, SendMessage, PostMessage) is completely unaffected
because it doesn't go through UIA/COM.

## What Works

### Finding dialogs
- `EnumWindows` + title/class matching — always works, never blocks
- QMessageBox: top-level HWND, class `Qt6102QWindowIcon`, has NO Win32 children (Qt draws them)
- QFileDialog (native): top-level HWND, class `#32770`, has FULL Win32 child tree

### Closing QMessageBox (About dialog)
- `WM_CLOSE` — works (equivalent to X button)
- `PostMessageW(hwnd, WM_KEYDOWN, VK_RETURN, 0)` — works! Qt routes Enter to the default button (OK)
- `BM_CLICK` on OK button — DOES NOT WORK because Qt doesn't expose QPushButton as Win32 HWND children
- pywinauto `.invoke()`, `.click()` — BLOCKS forever (UIA frozen)

### Closing/driving native file dialog (Export Data)
- `WM_CLOSE` — works
- `BM_CLICK` on Cancel/Save buttons — WORKS! Native file dialog exposes real Win32 Button children
- `WM_SETTEXT` on Edit control — WORKS! Can type filenames
- `WM_GETTEXT` on Edit control — WORKS! Can read filenames back
- `CB_GETLBTEXT` / `CB_SETCURSEL` on ComboBox — WORKS! Can read/select file type filter
- `get_address_bar_path()` — reads ToolbarWindow32 "Address: ..." to get current folder
- Full child tree available: Edit, ComboBox, Button (&Save, Cancel, &Help), SysTreeView32, SHELLDLL_DefView

### Opening modal dialogs without blocking
- `menu_select('Help->About')` BLOCKS because the modal blocks Qt's event loop
- Solution: run `menu_select` in a `threading.Thread(daemon=True)` — returns immediately
- Keyboard shortcuts (`type_keys("^e")`) may also block — use threading

### Navigating folders
- Type a folder name (relative or absolute) in the filename Edit + press Enter → navigates
- Works: "build", "..", "C:\", absolute paths
- Caveat: if the typed text is NOT a folder, it tries to Save a file with that name!
- SysTreeView32 exists (39 items) but reading item text requires VirtualAllocEx (cross-process)
  → typing paths is simpler and sufficient

### File type filter (ComboBox)
- Found via: visible ComboBox with items containing "*" patterns
- Items: `["JSON Files (*.json)", "All Files (*)"]`
- CB_GETCURSEL / CB_SETCURSEL to read/change selection
- CB_GETCOUNT / CB_GETLBTEXT to enumerate items

## Key Win32 child controls in native file dialog (#32770)

From exploration of Export Data dialog:
```
Edit (filename field)     — WM_SETTEXT/WM_GETTEXT
Edit (address bar, inside ComboBoxEx32 chain — invisible)
ComboBox [0] (address bar dropdown)
ComboBox [1] (file type filter — "JSON Files (*.json)", "All Files (*)")
Button "&Save"
Button "Cancel"
Button "&Help"
SHELLDLL_DefView          (file list area)
SysTreeView32             (navigation pane / folder tree — 39 items)
ToolbarWindow32           "Navigation buttons" (back/forward)
ToolbarWindow32           "Address: C:\..." (breadcrumb — read current path)
ToolbarWindow32           "Up band" (go up one level)
ToolbarWindow32           "Address band"
```

## Experiments log

1. `desktop.window(title_re="About.*")` — can't find it (not top-level in UIA tree)
2. `app.child_window(title_re="About.*")` — blocks (UIA frozen by modal)
3. `Application(backend='uia').connect().top_window()` — blocks
4. `Application(backend='uia').connect().window(title_re="About.*")` — blocks
5. `Desktop.windows()` — blocks while modal is open!
6. Win32 `EnumWindows` — works, finds dialog as top-level HWND ✅
7. Win32 `EnumChildWindows` on QMessageBox — 0 children (Qt widgets aren't Win32 HWNDs)
8. Win32 `EnumChildWindows` on #32770 file dialog — full child tree ✅
9. `PostMessageW(VK_RETURN)` on QMessageBox — closes it (Enter → default button) ✅
10. `BM_CLICK` on native file dialog Cancel — closes it ✅
11. `WM_SETTEXT` on Edit in file dialog — types filename ✅
12. `WM_GETTEXT` on Edit in file dialog — reads filename back ✅
13. `get_address_bar_path()` — reads current folder from toolbar ✅
14. `CB_GETLBTEXT` on file type ComboBox — reads filter items ✅
15. `CB_SETCURSEL` on file type ComboBox — changes selection ✅
16. Navigate by typing path + Enter — navigates to "web", "..", "C:\" ✅
17. Navigate to non-existent folder — tries to save (closes dialog!) ⚠️

## What's built so far

### win32_helpers.py — low-level Win32 API helpers
- `find_window(title_contains, timeout)` — polls EnumWindows for a window
- `find_child(parent, text, class_name)` — finds a child by exact text/class
- `find_child_containing(parent, text_contains, class_name)` — partial match
- `click_button(parent, text)` — finds Button child and sends BM_CLICK
- `press_key(hwnd, vk_code)` — PostMessage keydown+keyup
- `set_edit_text(parent, text)` — WM_SETTEXT on first visible Edit child
- `get_edit_text(parent)` — WM_GETTEXT from first visible Edit child
- `get_address_bar_path(dialog)` — reads current folder from toolbar
- `get_file_type_combo(dialog)` — finds the file type filter ComboBox
- `get_combo_items(combo)` / `get_combo_selection` / `set_combo_selection`
- `navigate_to_folder(dialog, path)` — types path + Enter
- `close_window(hwnd)` — WM_CLOSE
- `close_windows_by_title(*patterns)` — safety net cleanup
- `is_visible(hwnd)`

### TODO
- [x] Navigate folders in the file dialog
- [x] Read the current directory from the address bar
- [x] Select file type from the ComboBox
- [ ] Build FileDialog helper class wrapping all of the above
- [ ] Build QtDialog helper for QMessageBox (press_key + find_window)
- [ ] Test Save button (actually saves a file, then delete it)
- [ ] Rewrite conftest.py with proper fixtures
- [ ] Rewrite test_menu_bar.py
- [ ] Rewrite test_keyboard_shortcuts.py
- [ ] Rewrite test_window.py (check if it needs changes)
- [ ] Update docs (05-tools.md, 04-testing.md) with new patterns
