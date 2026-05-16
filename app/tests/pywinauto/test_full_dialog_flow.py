"""Curated pywinauto suite — one test per unique shipped helper.

Patterns follow the originals (pre-trim test_full_dialog_flow.py + deleted
test_menu_bar.py).

Stranded helpers (kept in win32_helpers / native_dialogs for templates
that restore a native Save dialog surface, not demoed here):
  - file_types >= 2, selected_file_type_index, select_file_type
    (get_file_type_combo / get_combo_items / get_combo_selection /
    set_combo_selection) — folder picker has no file-type combo.
  - FileDialog.save() — File->Save now routes through the bridge to React.
  - set_edit_text / get_edit_text via set_filename / get_filename —
    designed for the Save dialog's single "File name" Edit. The Win11
    folder picker exposes multiple Edits (visible folder-name + hidden
    address-bar combo Edit), so find_child(class_name="Edit") is not
    deterministic on this surface.
"""

import os
import time

from native_dialogs import FileDialog, QtMessageBox, open_modal
from win32_helpers import is_visible


# ── About dialog (custom QDialog, Qt-drawn buttons) ──────────────────

def test_about_dialog(app):
    """Demo: QtMessageBox + press_ok (VK_RETURN) + title + is_open."""
    open_modal(app, "Help->About")
    time.sleep(0.5)

    with QtMessageBox("About") as dlg:
        assert dlg.is_open
        assert "About" in dlg.title
        dlg.press_ok()
        time.sleep(0.3)
        assert not dlg.is_open


def test_about_dialog_press_cancel(app):
    """Demo: QtMessageBox.press_cancel — VK_ESCAPE closes a custom QDialog."""
    open_modal(app, "Help->About")
    time.sleep(0.5)

    with QtMessageBox("About") as dlg:
        assert dlg.is_open
        dlg.press_cancel()
        time.sleep(0.3)
        assert not dlg.is_open


# ── Open Folder native dialog (#32770) ───────────────────────────────

def test_open_folder_dialog(app):
    """Demo: FileDialog + file_types == [] + click_button cancel + is_open."""
    open_modal(app, "File->Open Folder...")
    time.sleep(1)

    with FileDialog("Open Folder") as dlg:
        assert dlg.is_open
        # Folder picker has no file type filter
        assert dlg.file_types == []
        dlg.cancel()
        time.sleep(0.3)
        assert not dlg.is_open


def test_open_folder_press_escape(app):
    """Demo: FileDialog.press_escape — VK_ESCAPE on native dialog."""
    open_modal(app, "File->Open Folder...")
    time.sleep(1)

    with FileDialog("Open Folder") as dlg:
        assert dlg.is_open
        dlg.press_escape()
        time.sleep(0.3)
        assert not dlg.is_open


def test_open_folder_select_button(app):
    """Demo: FileDialog.select_folder — click the named "Select Folder" button."""
    open_modal(app, "File->Open Folder...")
    time.sleep(1)

    with FileDialog("Open Folder") as dlg:
        assert dlg.is_open
        dlg.select_folder()
        time.sleep(0.5)
        assert not dlg.is_open


def test_open_folder_navigate(app):
    """Demo: FileDialog.navigate — type-path-then-Enter; selects on a folder picker."""
    project_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

    app.set_focus()
    open_modal(app, "File->Open Folder...")
    time.sleep(1.5)

    with FileDialog("Open Folder") as dlg:
        original_hwnd = dlg.hwnd
        assert is_visible(original_hwnd)
        dlg.navigate(project_dir)
        time.sleep(1.5)
        # Typing a valid folder path + Enter selects the folder and closes
        # the picker. (The app then shows a confirmation popup titled
        # "Open Folder" — we assert on the original picker hwnd, not on
        # dlg.is_open, since dlg may have re-targeted to that popup.)
        assert not is_visible(original_hwnd)


def test_open_folder_address_bar(app):
    """Demo: get_address_bar_path / FileDialog.current_folder — read the folder picker's address bar."""
    open_modal(app, "File->Open Folder...")
    time.sleep(1)

    with FileDialog("Open Folder") as dlg:
        assert dlg.is_open
        current = dlg.current_folder
        # Address bar is a ToolbarWindow32 with text like
        # "Address: C:\\some\\path" — current_folder strips the prefix.
        assert current is not None, "Address bar should expose a path"
        assert ":" in current, f"Expected a Windows path with drive letter, got {current!r}"
        dlg.cancel()
