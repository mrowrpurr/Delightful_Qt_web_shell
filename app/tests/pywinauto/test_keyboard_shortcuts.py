"""Keyboard-shortcut mechanics — one test per unique pywinauto capability.

Modal dialogs block pywinauto's UIA backend, so keyboard input that opens
a modal must run in a thread. Non-modal windows (DevTools) work with UIA
directly.

Ctrl+S is not demoed here because File->Save no longer opens a native
QFileDialog in this template (it routes through the bridge to React).
"""

import threading
import time

from pywinauto import Desktop

from native_dialogs import FileDialog
from win32_helpers import close_windows_by_title


def test_ctrl_o_opens_folder_dialog(app):
    """Demo: type_keys (threaded) → modal — Ctrl+O fires the Open Folder shortcut."""
    app.set_focus()
    threading.Thread(target=lambda: app.type_keys("^o"), daemon=True).start()
    time.sleep(1)

    with FileDialog("Open Folder") as dlg:
        assert dlg.is_open
        dlg.cancel()
        time.sleep(0.3)
        assert not dlg.is_open


def test_f12_opens_dev_tools(app):
    """Demo: UIA backend on a non-modal top-level — F12 opens DevTools."""
    app.set_focus()
    app.type_keys("{F12}")
    time.sleep(1)

    dev_tools = Desktop(backend="uia").window(title_re=".*Developer Tools.*")
    assert dev_tools.exists()

    close_windows_by_title("Developer Tools")
    time.sleep(0.3)
