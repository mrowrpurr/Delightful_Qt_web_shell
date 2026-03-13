"""Figure out which Edit control is the filename field."""

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
    find_window, close_window, EnumChildWindows, GetWindowTextW,
    GetWindowTextLengthW, GetClassNameW, SendMessageW, WNDENUMPROC,
    WM_SETTEXT, WM_GETTEXT, WM_GETTEXTLENGTH, IsWindowVisible,
)

GetParent = ctypes.windll.user32.GetParent

d = Desktop(backend="uia")
app = d.window(title="Delightful Qt Web Shell")
app.wait("visible", timeout=5)
app.set_focus()

threading.Thread(target=lambda: app.menu_select("File->Export..."), daemon=True).start()
time.sleep(1.5)

dlg = find_window("Export Data")
print(f"Dialog hwnd={dlg}")

# Find ALL Edit controls and their parents
edits = []
def find_edits(hwnd, _):
    cls = ctypes.create_unicode_buffer(256)
    GetClassNameW(hwnd, cls, 256)
    if cls.value == "Edit":
        parent = GetParent(hwnd)
        parent_cls = ctypes.create_unicode_buffer(256)
        GetClassNameW(parent, parent_cls, 256)
        parent_text_len = GetWindowTextLengthW(parent)
        parent_text_buf = ctypes.create_unicode_buffer(max(parent_text_len + 1, 1))
        GetWindowTextW(parent, parent_text_buf, parent_text_len + 1)

        vis = IsWindowVisible(hwnd)
        edits.append((hwnd, parent, parent_cls.value, parent_text_buf.value, vis))
    return True

EnumChildWindows(dlg, WNDENUMPROC(find_edits), 0)

print(f"\nFound {len(edits)} Edit controls:")
for i, (hwnd, parent, pcls, ptxt, vis) in enumerate(edits):
    print(f"  [{i}] hwnd={hwnd} parent_hwnd={parent} parent_class={pcls!r} parent_text={ptxt!r} visible={vis}")

# Try writing to each visible Edit and reading back
print("\n--- Testing each Edit ---")
test_text = "test_file.json"
for i, (hwnd, parent, pcls, ptxt, vis) in enumerate(edits):
    if not vis:
        print(f"  [{i}] SKIPPED (not visible)")
        continue

    # Write
    SendMessageW(hwnd, WM_SETTEXT, 0, ctypes.c_wchar_p(test_text))

    # Read back
    length = SendMessageW(hwnd, WM_GETTEXTLENGTH, 0, 0)
    buf = ctypes.create_unicode_buffer(length + 1)
    SendMessageW(hwnd, WM_GETTEXT, length + 1, buf)

    print(f"  [{i}] hwnd={hwnd} parent_class={pcls!r} wrote={test_text!r} readback={buf.value!r}")

# Clean up: figure out which one actually has the text in the "File name:" field
# The filename Edit is typically inside a ComboBoxEx32 > ComboBox > Edit chain
print("\n--- Identifying filename Edit ---")
for i, (hwnd, parent, pcls, ptxt, vis) in enumerate(edits):
    if pcls == "ComboBox":
        # Check if grandparent is ComboBoxEx32
        grandparent = GetParent(parent)
        gp_cls = ctypes.create_unicode_buffer(256)
        GetClassNameW(grandparent, gp_cls, 256)
        print(f"  [{i}] ComboBox parent chain: Edit -> ComboBox(hwnd={parent}) -> {gp_cls.value}(hwnd={grandparent})")
        if gp_cls.value == "ComboBoxEx32":
            print(f"  ^^^ THIS IS THE FILENAME EDIT (ComboBoxEx32 > ComboBox > Edit)")

close_window(dlg)
time.sleep(0.3)
print(f"\nClosed. Done!")
