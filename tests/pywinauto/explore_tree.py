"""Explore the folder navigation tree (SysTreeView32) in the file dialog."""

import ctypes
import ctypes.wintypes as w
import sys
import io
import time
import threading

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
sys.stdout.reconfigure(line_buffering=True)

from pywinauto import Desktop
from win32_helpers import (
    find_window, find_child, close_window, click_button,
    EnumChildWindows, GetClassNameW, SendMessageW, WNDENUMPROC,
    get_address_bar_path,
)

# TreeView messages
TV_FIRST = 0x1100
TVM_GETCOUNT = TV_FIRST + 5
TVM_GETNEXTITEM = TV_FIRST + 10
TVM_GETITEMW = TV_FIRST + 62
TVM_SELECTITEM = TV_FIRST + 11
TVM_EXPAND = TV_FIRST + 2

TVGN_ROOT = 0x0000
TVGN_NEXT = 0x0001
TVGN_CHILD = 0x0004
TVGN_CARET = 0x0009  # currently selected

TVE_EXPAND = 0x0002

d = Desktop(backend="uia")
app = d.window(title="Delightful Qt Web Shell")
app.wait("visible", timeout=5)
app.set_focus()

threading.Thread(target=lambda: app.menu_select("File->Export..."), daemon=True).start()
time.sleep(1.5)

dlg = find_window("Export Data")
print(f"Dialog hwnd={dlg}")

# Find the SysTreeView32
tree = find_child(dlg, class_name="SysTreeView32")
print(f"TreeView hwnd={tree}")

if tree:
    count = SendMessageW(tree, TVM_GETCOUNT, 0, 0)
    print(f"Item count: {count}")

    # Get root item
    root = SendMessageW(tree, TVM_GETNEXTITEM, TVGN_ROOT, 0)
    print(f"Root HTREEITEM: {root}")

    # Get currently selected item
    selected = SendMessageW(tree, TVM_GETNEXTITEM, TVGN_CARET, 0)
    print(f"Selected HTREEITEM: {selected}")

    # NOTE: Reading tree item text requires shared memory (VirtualAllocEx)
    # because TVM_GETITEM needs a TVITEM struct in the target process's address space.
    # This is complex but doable. For now, let's try a simpler approach:
    # navigate by typing the path directly in the filename Edit field.

    print("\n--- Alternative: navigate by typing path in filename ---")
    print(f"Current path: {get_address_bar_path(dlg)!r}")

    # Type a relative path to navigate
    from win32_helpers import set_edit_text, press_key, VK_RETURN
    set_edit_text(dlg, "web")
    press_key(dlg, VK_RETURN)
    time.sleep(1)

    # Re-find dialog (might have changed)
    try:
        dlg = find_window("Export Data", timeout=2)
        print(f"After navigating to 'web': {get_address_bar_path(dlg)!r}")

        # Navigate back up
        set_edit_text(dlg, "..")
        press_key(dlg, VK_RETURN)
        time.sleep(1)

        dlg = find_window("Export Data", timeout=2)
        print(f"After navigating to '..': {get_address_bar_path(dlg)!r}")

        # Navigate to absolute path
        set_edit_text(dlg, "C:\\")
        press_key(dlg, VK_RETURN)
        time.sleep(1)

        dlg = find_window("Export Data", timeout=2)
        print(f"After navigating to 'C:\\': {get_address_bar_path(dlg)!r}")

    except Exception as e:
        print(f"Error: {e}")

    # Clean up
    try:
        dlg = find_window("Export Data", timeout=2)
        click_button(dlg, "Cancel")
    except:
        pass

print("\nDone! 🔥")
