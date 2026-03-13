"""High-level helpers for driving native Windows dialogs from pywinauto tests.

Qt6 modal dialogs block pywinauto's UIA backend completely. These classes use
the Win32 API directly to find and interact with modal dialogs while they're
open. They're designed to be used alongside pywinauto for non-modal interactions
(menus, keyboard shortcuts, window properties).

Usage:
    from native_dialogs import FileDialog, QtMessageBox, open_modal

    # Open a modal dialog (menu_select in a thread to avoid blocking)
    open_modal(app, "File->Export...")

    # Drive a native Windows file dialog
    with FileDialog("Export Data") as dlg:
        dlg.set_filename("my_data.json")
        dlg.navigate("C:/Users/Desktop")
        assert "JSON Files" in dlg.file_types[0]
        dlg.cancel()

    # Drive a Qt QMessageBox
    with QtMessageBox("About") as dlg:
        dlg.press_ok()  # Enter key → default button
"""

import os
import threading
import time

from win32_helpers import (
    find_window, find_child, find_child_containing, click_button,
    press_key, set_edit_text, get_edit_text, get_address_bar_path,
    get_file_type_combo, get_combo_items, get_combo_selection,
    set_combo_selection, navigate_to_folder, close_window, is_visible,
    close_windows_by_title, VK_RETURN, VK_ESCAPE,
)


def open_modal(app, menu_path):
    """Open a menu item that triggers a modal dialog.

    Runs menu_select in a daemon thread because modal dialogs block Qt's event
    loop, which blocks menu_select from returning.
    """
    threading.Thread(target=lambda: app.menu_select(menu_path), daemon=True).start()


class FileDialog:
    """Drive a native Windows file dialog (Save/Open).

    The native file dialog (class #32770) is fully drivable via Win32 API:
    buttons, filename field, file type filter, folder navigation, address bar.

    Can be used as a context manager — automatically closes the dialog on exit
    if it's still open (safety net for test cleanup).
    """

    def __init__(self, title_contains, timeout=5):
        self.hwnd = find_window(title_contains, timeout=timeout)
        self._title_contains = title_contains

    def __enter__(self):
        return self

    def __exit__(self, *_):
        if is_visible(self.hwnd):
            close_window(self.hwnd)
            time.sleep(0.3)

    # ── Filename ──────────────────────────────────────────────────────

    def set_filename(self, name):
        """Type a filename into the file name field."""
        set_edit_text(self.hwnd, name)

    def get_filename(self):
        """Read the current filename from the file name field."""
        return get_edit_text(self.hwnd)

    # ── Buttons ───────────────────────────────────────────────────────

    def save(self):
        """Click the Save button."""
        click_button(self.hwnd, "Save")

    def cancel(self):
        """Click the Cancel button."""
        click_button(self.hwnd, "Cancel")

    def press_escape(self):
        """Press Escape to cancel (same as clicking Cancel)."""
        press_key(self.hwnd, VK_ESCAPE)

    # ── Navigation ────────────────────────────────────────────────────

    @property
    def current_folder(self):
        """The current folder path shown in the address bar."""
        return get_address_bar_path(self.hwnd)

    def navigate(self, path):
        """Navigate to a folder by typing a path and pressing Enter.

        Supports relative ("subfolder"), parent (".."), and absolute ("C:\\Users")
        paths. Forward slashes are converted to backslashes automatically.
        The path must be an existing directory — if it's not, the dialog
        will try to save a file with that name.
        """
        navigate_to_folder(self.hwnd, os.path.normpath(path))
        time.sleep(0.5)
        # Re-find in case HWND changed
        try:
            self.hwnd = find_window(self._title_contains, timeout=2)
        except TimeoutError:
            pass  # dialog may have closed (if path wasn't a folder)

    # ── File type filter ──────────────────────────────────────────────

    @property
    def file_types(self):
        """List of available file type filter options."""
        combo = get_file_type_combo(self.hwnd)
        return get_combo_items(combo) if combo else []

    @property
    def selected_file_type_index(self):
        """Index of the currently selected file type."""
        combo = get_file_type_combo(self.hwnd)
        return get_combo_selection(combo) if combo else -1

    def select_file_type(self, index):
        """Select a file type by index."""
        combo = get_file_type_combo(self.hwnd)
        if combo:
            set_combo_selection(combo, index)

    # ── State ─────────────────────────────────────────────────────────

    @property
    def is_open(self):
        """Whether the dialog is still visible."""
        return is_visible(self.hwnd)


class QtMessageBox:
    """Drive a Qt QMessageBox dialog.

    QMessageBox has NO Win32 child controls — Qt draws the buttons itself.
    The only way to interact is via keyboard: Enter presses the default button
    (usually OK), Escape closes the dialog (Cancel/Close).
    """

    def __init__(self, title_contains, timeout=3):
        self.hwnd = find_window(title_contains, timeout=timeout)
        self._title_contains = title_contains

    def __enter__(self):
        return self

    def __exit__(self, *_):
        if is_visible(self.hwnd):
            close_window(self.hwnd)
            time.sleep(0.3)

    def press_ok(self):
        """Press Enter to click the default button (usually OK)."""
        press_key(self.hwnd, VK_RETURN)

    def press_cancel(self):
        """Press Escape to cancel/close."""
        press_key(self.hwnd, VK_ESCAPE)

    def close(self):
        """Send WM_CLOSE (equivalent to the X button)."""
        close_window(self.hwnd)

    @property
    def title(self):
        """The dialog's window title."""
        from win32_helpers import _get_text
        return _get_text(self.hwnd)

    @property
    def is_open(self):
        """Whether the dialog is still visible."""
        return is_visible(self.hwnd)
