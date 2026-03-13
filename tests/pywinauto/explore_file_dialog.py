"""Explore the native Windows file dialog controls.

Run: uv run python tests/pywinauto/explore_file_dialog.py
Requires: xmake run start-desktop (app must be running)
"""

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
    find_window, find_child, find_child_containing, click_button,
    press_key, set_edit_text, get_edit_text, close_window, is_visible,
    close_windows_by_title, EnumChildWindows, GetWindowTextW,
    GetWindowTextLengthW, GetClassNameW, SendMessageW, WNDENUMPROC,
    VK_RETURN, VK_ESCAPE, BM_CLICK,
)


def get_app():
    d = Desktop(backend="uia")
    app = d.window(title="Delightful Qt Web Shell")
    app.wait("visible", timeout=5)
    app.set_focus()
    return app


def open_export(app):
    """Open File > Export... in a thread (modal blocks menu_select)."""
    threading.Thread(target=lambda: app.menu_select("File->Export..."), daemon=True).start()
    time.sleep(1.5)
    return find_window("Export Data")


def open_about(app):
    """Open Help > About in a thread."""
    threading.Thread(target=lambda: app.menu_select("Help->About"), daemon=True).start()
    time.sleep(0.5)
    return find_window("About")


def dump_children(hwnd, indent=0):
    """Print all children of a window recursively (direct children only)."""
    children = []

    def callback(child_hwnd, _):
        length = GetWindowTextLengthW(child_hwnd)
        buf = ctypes.create_unicode_buffer(max(length + 1, 1))
        GetWindowTextW(child_hwnd, buf, length + 1)
        cls = ctypes.create_unicode_buffer(256)
        GetClassNameW(child_hwnd, cls, 256)
        children.append((child_hwnd, buf.value, cls.value))
        return True

    EnumChildWindows(hwnd, WNDENUMPROC(callback), 0)

    for child_hwnd, text, cls in children:
        prefix = "  " * indent
        label = f"{text!r}" if text else "(empty)"
        print(f"{prefix}hwnd={child_hwnd} class={cls!r} text={label}")


def get_address_bar_path(dialog_hwnd):
    """Read the current folder path from the address bar toolbar."""
    result = find_child_containing(dialog_hwnd, "Address:", class_name="ToolbarWindow32")
    if result:
        length = GetWindowTextLengthW(result)
        buf = ctypes.create_unicode_buffer(length + 1)
        GetWindowTextW(result, buf, length + 1)
        # Text is like "Address: C:\some\path"
        text = buf.value
        if text.startswith("Address: "):
            return text[len("Address: "):]
        return text
    return None


# ── Main exploration ─────────────────────────────────────────────────────

if __name__ == "__main__":
    app = get_app()

    print("=" * 60)
    print("EXPERIMENT 1: Open Export, dump controls, type filename, Cancel")
    print("=" * 60)

    dlg = open_export(app)
    print(f"\nDialog hwnd={dlg}")

    print("\n--- All children ---")
    dump_children(dlg)

    print(f"\n--- Address bar: {get_address_bar_path(dlg)!r} ---")

    print("\n--- Setting filename to 'my_export.json' ---")
    set_edit_text(dlg, "my_export.json")
    print(f"Read back: {get_edit_text(dlg)!r}")

    print("\n--- Clicking Cancel ---")
    click_button(dlg, "Cancel")
    time.sleep(0.3)
    print(f"Dialog still visible? {is_visible(dlg)}")

    print("\n" + "=" * 60)
    print("EXPERIMENT 2: Open About, press Enter to close")
    print("=" * 60)

    dlg = open_about(app)
    print(f"\nDialog hwnd={dlg}")

    print("\n--- Pressing Enter (OK is default button) ---")
    press_key(dlg, VK_RETURN)
    time.sleep(0.3)
    print(f"Dialog still visible? {is_visible(dlg)}")

    print("\n" + "=" * 60)
    print("EXPERIMENT 3: Open Export, navigate to a subfolder")
    print("=" * 60)

    dlg = open_export(app)
    print(f"Current path: {get_address_bar_path(dlg)!r}")

    # Type a path directly into the filename field — Windows file dialog
    # interprets paths in the filename field!
    print("\n--- Typing 'build' into filename to navigate ---")
    set_edit_text(dlg, "build")

    # The Open button navigates to folders, Save saves files
    # For the Save dialog, typing a folder name and pressing Enter navigates there
    # Let's try pressing Enter
    press_key(dlg, VK_RETURN)
    time.sleep(1)

    # Check if we navigated (dialog should still be open, path changed)
    if is_visible(dlg):
        # Need to re-find because the dialog HWND might change
        try:
            dlg2 = find_window("Export Data", timeout=2)
            print(f"New path: {get_address_bar_path(dlg2)!r}")
            print("Navigated successfully!" if "build" in (get_address_bar_path(dlg2) or "") else "Navigation unclear")

            # Clean up
            click_button(dlg2, "Cancel")
        except Exception as e:
            print(f"Re-find failed: {e}")
            close_windows_by_title("Export Data")
    else:
        print("Dialog closed (might have tried to save 'build' as a file?)")

    print("\nDone! 🔥")
