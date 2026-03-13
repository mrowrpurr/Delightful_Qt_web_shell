"""Test that keyboard shortcuts trigger the correct native actions.

Modal dialogs block pywinauto's UIA backend, so we use threading + Win32 API
to drive dialogs opened by keyboard shortcuts.
"""

import threading
import time

from assertpy import assert_that

from native_dialogs import FileDialog
from win32_helpers import close_windows_by_title


def test_ctrl_e_opens_export_dialog(app):
    """Ctrl+E should open the Export file dialog."""
    app.set_focus()
    # type_keys may block when the modal opens — run in a thread
    threading.Thread(target=lambda: app.type_keys("^e"), daemon=True).start()
    time.sleep(1)

    with FileDialog("Export Data") as dlg:
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
