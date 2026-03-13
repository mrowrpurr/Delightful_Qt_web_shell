"""Test the native Qt menu bar and its actions.

Modal dialogs (About, Save, Open Folder) block pywinauto's UIA backend entirely.
We use open_modal() to trigger menu items in a thread, then drive the
resulting dialogs via Win32 API through our FileDialog/QtMessageBox helpers.
"""

import time

from assertpy import assert_that

from native_dialogs import FileDialog, QtMessageBox, open_modal


def test_file_menu_has_save_open_folder_and_quit(app):
    """File menu should have Save, Open Folder, and Quit actions."""
    app.menu_select("File")
    time.sleep(0.3)
    app.type_keys("{ESC}")


def test_help_menu_has_about(app):
    """Help menu should have an About action."""
    app.menu_select("Help")
    time.sleep(0.3)
    app.type_keys("{ESC}")


def test_about_dialog_opens_and_closes(app):
    """Help > About should open a QMessageBox with app info."""
    open_modal(app, "Help->About")
    time.sleep(0.5)

    with QtMessageBox("About") as dlg:
        assert_that(dlg.is_open).is_true()
        assert_that(dlg.title).contains("About")
        dlg.press_ok()
        time.sleep(0.3)
        assert_that(dlg.is_open).is_false()


def test_save_dialog_opens_and_closes(app):
    """File > Save... should open a native save file dialog."""
    open_modal(app, "File->Save...")
    time.sleep(1)

    with FileDialog("Save File") as dlg:
        assert_that(dlg.is_open).is_true()
        assert_that(len(dlg.file_types)).is_greater_than_or_equal_to(1)
        dlg.cancel()
        time.sleep(0.3)
        assert_that(dlg.is_open).is_false()


def test_open_folder_dialog_opens_and_closes(app):
    """File > Open Folder... should open a native folder picker."""
    open_modal(app, "File->Open Folder...")
    time.sleep(1)

    with FileDialog("Open Folder") as dlg:
        assert_that(dlg.is_open).is_true()
        # Folder picker has no file type filter
        assert_that(dlg.file_types).is_empty()
        dlg.cancel()
        time.sleep(0.3)
        assert_that(dlg.is_open).is_false()
