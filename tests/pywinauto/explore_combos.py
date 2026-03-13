"""Explore ComboBox controls in the file dialog — particularly the file type filter."""

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
    find_window, close_window, find_child, EnumChildWindows, GetWindowTextW,
    GetWindowTextLengthW, GetClassNameW, SendMessageW, IsWindowVisible,
    WNDENUMPROC, click_button,
)

GetParent = ctypes.windll.user32.GetParent

CB_GETCOUNT = 0x0146
CB_GETCURSEL = 0x0147
CB_GETLBTEXT = 0x0148
CB_GETLBTEXTLEN = 0x0149
CB_SETCURSEL = 0x014E
CB_SHOWDROPDOWN = 0x014F

d = Desktop(backend="uia")
app = d.window(title="Delightful Qt Web Shell")
app.wait("visible", timeout=5)
app.set_focus()

threading.Thread(target=lambda: app.menu_select("File->Export..."), daemon=True).start()
time.sleep(1.5)

dlg = find_window("Export Data")
print(f"Dialog hwnd={dlg}")

# Find ALL ComboBox controls
combos = []
def find_combos(hwnd, _):
    cls = ctypes.create_unicode_buffer(256)
    GetClassNameW(hwnd, cls, 256)
    if cls.value == "ComboBox":
        parent = GetParent(hwnd)
        parent_cls = ctypes.create_unicode_buffer(256)
        GetClassNameW(parent, parent_cls, 256)
        vis = IsWindowVisible(hwnd)
        combos.append((hwnd, parent, parent_cls.value, vis))
    return True

EnumChildWindows(dlg, WNDENUMPROC(find_combos), 0)

print(f"\nFound {len(combos)} ComboBox controls:")
for i, (hwnd, parent, pcls, vis) in enumerate(combos):
    count = SendMessageW(hwnd, CB_GETCOUNT, 0, 0)
    cur = SendMessageW(hwnd, CB_GETCURSEL, 0, 0)
    print(f"  [{i}] hwnd={hwnd} parent_class={pcls!r} visible={vis} items={count} selected={cur}")

    if count > 0 and count < 20:  # Don't dump huge lists
        for j in range(count):
            text_len = SendMessageW(hwnd, CB_GETLBTEXTLEN, j, 0)
            if text_len > 0 and text_len < 500:
                buf = ctypes.create_unicode_buffer(text_len + 1)
                SendMessageW(hwnd, CB_GETLBTEXT, j, buf)
                marker = " <--" if j == cur else ""
                print(f"      [{j}] {buf.value!r}{marker}")

# The file type filter ComboBox should have items like "JSON Files (*.json)" and "All Files (*)"
# Let's try selecting a different item
print("\n--- Looking for file type ComboBox ---")
for i, (hwnd, parent, pcls, vis) in enumerate(combos):
    count = SendMessageW(hwnd, CB_GETCOUNT, 0, 0)
    if count >= 2 and vis:
        # Read items to check if this is the file type filter
        items = []
        for j in range(count):
            text_len = SendMessageW(hwnd, CB_GETLBTEXTLEN, j, 0)
            if text_len > 0:
                buf = ctypes.create_unicode_buffer(text_len + 1)
                SendMessageW(hwnd, CB_GETLBTEXT, j, buf)
                items.append(buf.value)

        if any("*" in item for item in items):
            print(f"  Found file type filter at [{i}] hwnd={hwnd}!")
            print(f"  Items: {items}")

            # Select "All Files (*)" if available
            for j, item in enumerate(items):
                if "All Files" in item:
                    print(f"  Selecting [{j}] {item!r}...")
                    SendMessageW(hwnd, CB_SETCURSEL, j, 0)
                    time.sleep(0.3)
                    new_sel = SendMessageW(hwnd, CB_GETCURSEL, 0, 0)
                    print(f"  New selection: [{new_sel}]")
            break

click_button(dlg, "Cancel")
print("\nDone!")
