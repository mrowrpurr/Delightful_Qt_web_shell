"""Win32 API helpers for interacting with modal dialogs.

Qt6 modal dialogs block the UIA COM server, making pywinauto's UIA backend
completely unusable while a modal is open. These helpers use the Win32 API
directly via ctypes — EnumWindows, SendMessage, PostMessage — which are
unaffected by Qt's modal event loop.

Usage:
    from win32_helpers import find_window, click_button, press_key, set_edit_text

    hwnd = find_window("Export Data")          # find by title
    click_button(hwnd, "Cancel")               # click a button by text
    press_key(hwnd, VK_RETURN)                 # press Enter
    set_edit_text(hwnd, "test.json")           # type into the filename field
"""

import ctypes
import ctypes.wintypes as wintypes
import time

# ── Constants ────────────────────────────────────────────────────────────

WM_CLOSE = 0x0010
WM_SETTEXT = 0x000C
WM_GETTEXT = 0x000D
WM_GETTEXTLENGTH = 0x000E
WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
BM_CLICK = 0x00F5

VK_RETURN = 0x0D
VK_ESCAPE = 0x1B

# ── Win32 function bindings ──────────────────────────────────────────────

EnumWindows = ctypes.windll.user32.EnumWindows
EnumChildWindows = ctypes.windll.user32.EnumChildWindows
GetWindowTextW = ctypes.windll.user32.GetWindowTextW
GetWindowTextLengthW = ctypes.windll.user32.GetWindowTextLengthW
GetClassNameW = ctypes.windll.user32.GetClassNameW
SendMessageW = ctypes.windll.user32.SendMessageW
PostMessageW = ctypes.windll.user32.PostMessageW
SetForegroundWindow = ctypes.windll.user32.SetForegroundWindow
IsWindowVisible = ctypes.windll.user32.IsWindowVisible
WNDENUMPROC = ctypes.WINFUNCTYPE(ctypes.c_bool, wintypes.HWND, wintypes.LPARAM)


# ── Helpers ──────────────────────────────────────────────────────────────

def _get_text(hwnd):
    length = GetWindowTextLengthW(hwnd)
    if length == 0:
        return ""
    buf = ctypes.create_unicode_buffer(length + 1)
    GetWindowTextW(hwnd, buf, length + 1)
    return buf.value


def _get_class(hwnd):
    buf = ctypes.create_unicode_buffer(256)
    GetClassNameW(hwnd, buf, 256)
    return buf.value


def find_window(title_contains, timeout=5):
    """Find a top-level window whose title contains the given string.

    Polls until found or timeout. Returns the HWND, or raises if not found.
    """
    start = time.time()
    while time.time() - start < timeout:
        result = None

        def callback(hwnd, _):
            nonlocal result
            text = _get_text(hwnd)
            if title_contains in text and IsWindowVisible(hwnd):
                result = hwnd
                return False  # stop enumerating
            return True

        EnumWindows(WNDENUMPROC(callback), 0)
        if result:
            return result
        time.sleep(0.1)

    raise TimeoutError(f"Window with title containing {title_contains!r} not found after {timeout}s")


def find_child(parent_hwnd, text=None, class_name=None):
    """Find a child control by text and/or class name. Returns HWND or None."""
    result = None

    def callback(hwnd, _):
        nonlocal result
        if text is not None and _get_text(hwnd) != text:
            return True
        if class_name is not None and _get_class(hwnd) != class_name:
            return True
        result = hwnd
        return False  # found it

    EnumChildWindows(parent_hwnd, WNDENUMPROC(callback), 0)
    return result


def find_child_containing(parent_hwnd, text_contains, class_name=None):
    """Find a child control whose text contains the given string."""
    result = None

    def callback(hwnd, _):
        nonlocal result
        if text_contains not in _get_text(hwnd):
            return True
        if class_name is not None and _get_class(hwnd) != class_name:
            return True
        result = hwnd
        return False

    EnumChildWindows(parent_hwnd, WNDENUMPROC(callback), 0)
    return result


def click_button(parent_hwnd, button_text):
    """Find a Button child by text and send BM_CLICK."""
    # Button text may have & for accelerator (e.g. "&Save")
    btn = find_child(parent_hwnd, text=button_text, class_name="Button")
    if not btn:
        btn = find_child(parent_hwnd, text=f"&{button_text}", class_name="Button")
    if not btn:
        raise RuntimeError(f"Button {button_text!r} not found in hwnd={parent_hwnd}")
    SendMessageW(btn, BM_CLICK, 0, 0)


def press_key(hwnd, vk_code):
    """Send a keypress (down + up) to a window via PostMessage."""
    SetForegroundWindow(hwnd)
    time.sleep(0.05)
    PostMessageW(hwnd, WM_KEYDOWN, vk_code, 0)
    PostMessageW(hwnd, WM_KEYUP, vk_code, 0)


def set_edit_text(parent_hwnd, text):
    """Set text in the first Edit control inside a window."""
    edit = find_child(parent_hwnd, class_name="Edit")
    if not edit:
        raise RuntimeError(f"No Edit control found in hwnd={parent_hwnd}")
    SendMessageW(edit, WM_SETTEXT, 0, ctypes.c_wchar_p(text))


def get_edit_text(parent_hwnd):
    """Get text from the first Edit control inside a window."""
    edit = find_child(parent_hwnd, class_name="Edit")
    if not edit:
        raise RuntimeError(f"No Edit control found in hwnd={parent_hwnd}")
    length = SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0)
    buf = ctypes.create_unicode_buffer(length + 1)
    SendMessageW(edit, WM_GETTEXT, length + 1, buf)
    return buf.value


def get_address_bar_path(dialog_hwnd):
    """Read the current folder path from a file dialog's address bar.

    The address bar is a ToolbarWindow32 with text like "Address: C:\\some\\path".
    """
    toolbar = find_child_containing(dialog_hwnd, "Address:", class_name="ToolbarWindow32")
    if not toolbar:
        return None
    text = _get_text(toolbar)
    prefix = "Address: "
    return text[len(prefix):] if text.startswith(prefix) else text


def get_file_type_combo(dialog_hwnd):
    """Find the file type filter ComboBox in a file dialog.

    Returns the HWND of the ComboBox that contains file type options
    like "JSON Files (*.json)", "All Files (*)", etc.
    """
    CB_GETCOUNT = 0x0146
    CB_GETLBTEXTLEN = 0x0149
    CB_GETLBTEXT = 0x0148

    result = None

    def callback(hwnd, _):
        nonlocal result
        if _get_class(hwnd) != "ComboBox":
            return True
        if not IsWindowVisible(hwnd):
            return True
        count = SendMessageW(hwnd, CB_GETCOUNT, 0, 0)
        if count < 2:
            return True
        # Check if any item contains a wildcard pattern like "*."
        for i in range(count):
            text_len = SendMessageW(hwnd, CB_GETLBTEXTLEN, i, 0)
            if text_len > 0:
                buf = ctypes.create_unicode_buffer(text_len + 1)
                SendMessageW(hwnd, CB_GETLBTEXT, i, buf)
                if "*" in buf.value:
                    result = hwnd
                    return False
        return True

    EnumChildWindows(dialog_hwnd, WNDENUMPROC(callback), 0)
    return result


def get_combo_items(combo_hwnd):
    """Get all items from a ComboBox."""
    CB_GETCOUNT = 0x0146
    CB_GETLBTEXTLEN = 0x0149
    CB_GETLBTEXT = 0x0148

    count = SendMessageW(combo_hwnd, CB_GETCOUNT, 0, 0)
    items = []
    for i in range(count):
        text_len = SendMessageW(combo_hwnd, CB_GETLBTEXTLEN, i, 0)
        if text_len > 0:
            buf = ctypes.create_unicode_buffer(text_len + 1)
            SendMessageW(combo_hwnd, CB_GETLBTEXT, i, buf)
            items.append(buf.value)
        else:
            items.append("")
    return items


def get_combo_selection(combo_hwnd):
    """Get the currently selected index in a ComboBox."""
    CB_GETCURSEL = 0x0147
    return SendMessageW(combo_hwnd, CB_GETCURSEL, 0, 0)


def set_combo_selection(combo_hwnd, index):
    """Select an item in a ComboBox by index."""
    CB_SETCURSEL = 0x014E
    SendMessageW(combo_hwnd, CB_SETCURSEL, index, 0)


def navigate_to_folder(dialog_hwnd, path):
    """Navigate a file dialog to a folder by typing the path and pressing Enter.

    Works for both relative paths ("subfolder"), parent (".."),
    and absolute paths ("C:\\Users").
    """
    set_edit_text(dialog_hwnd, path)
    press_key(dialog_hwnd, VK_RETURN)


def close_window(hwnd):
    """Send WM_CLOSE to a window."""
    SendMessageW(hwnd, WM_CLOSE, 0, 0)


def is_visible(hwnd):
    """Check if a window is still visible."""
    return bool(IsWindowVisible(hwnd))


def close_windows_by_title(*patterns):
    """Close all top-level windows whose title contains any of the given patterns.

    Used as a safety net in test cleanup fixtures.
    """
    def callback(hwnd, _):
        text = _get_text(hwnd)
        for pattern in patterns:
            if pattern in text:
                SendMessageW(hwnd, WM_CLOSE, 0, 0)
                break
        return True

    EnumWindows(WNDENUMPROC(callback), 0)
