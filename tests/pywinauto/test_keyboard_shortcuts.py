"""Test that keyboard shortcuts trigger the correct native actions.

Modal dialogs block pywinauto's UIA backend, so we use threading + Win32 API
to drive dialogs opened by keyboard shortcuts.
"""

import threading
import time

from assertpy import assert_that

from native_dialogs import FileDialog
from win32_helpers import close_windows_by_title


def test_ctrl_s_opens_save_dialog(app):
    """Ctrl+S should open the Save file dialog."""
    app.set_focus()
    # type_keys may block when the modal opens — run in a thread
    threading.Thread(target=lambda: app.type_keys("^s"), daemon=True).start()
    time.sleep(1)

    with FileDialog("Save File") as dlg:
        assert_that(dlg.is_open).is_true()
        dlg.cancel()
        time.sleep(0.3)
        assert_that(dlg.is_open).is_false()


def test_ctrl_o_opens_folder_dialog(app):
    """Ctrl+O should open the Open Folder dialog."""
    app.set_focus()
    threading.Thread(target=lambda: app.type_keys("^o"), daemon=True).start()
    time.sleep(1)

    with FileDialog("Open Folder") as dlg:
        assert_that(dlg.is_open).is_true()
        dlg.cancel()
        time.sleep(0.3)
        assert_that(dlg.is_open).is_false()


def test_f12_opens_dev_tools(app):
    """F12 should open the Developer Tools window."""
    app.set_focus()
    app.type_keys("{F12}")
    time.sleep(1)

    # DevTools is a separate top-level non-modal window — UIA works fine
    from pywinauto import Desktop
    d = Desktop(backend="uia")
    dev_tools = d.window(title_re=".*Developer Tools.*")
    assert_that(dev_tools.exists()).is_true()

    close_windows_by_title("Developer Tools")
    time.sleep(0.3)
