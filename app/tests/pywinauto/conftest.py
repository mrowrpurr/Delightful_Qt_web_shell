"""Shared fixtures for pywinauto tests.

These tests require the desktop app to be running:
    xmake run app.demo.start

Run with:
    uv run pytest tests/pywinauto/
    # or
    xmake run app.test.automation
"""

import io
import sys
import time

import pytest
from pywinauto import Desktop

from win32_helpers import close_windows_by_title


# Tests print emoji (✅) for status. Windows console defaults to cp1252 and
# pytest's stdout capture inherits that, which raises UnicodeEncodeError.
# Reconfigure both streams to utf-8 with a replace fallback.
for _stream_name in ("stdout", "stderr"):
    _stream = getattr(sys, _stream_name)
    if hasattr(_stream, "reconfigure"):
        _stream.reconfigure(encoding="utf-8", errors="replace")


APP_CLASS = "DemoWindow"


@pytest.fixture
def desktop():
    return Desktop(backend="uia")


@pytest.fixture
def app(desktop):
    """Find the running Qt app window. Fails if the app isn't running."""
    try:
        window = desktop.window(class_name=APP_CLASS)
        window.wait("visible", timeout=5)
        return window
    except Exception:
        pytest.fail(
            f"App not found (class_name={APP_CLASS}). Is it running? Start it with: xmake run app.demo.start"
        )


@pytest.fixture(autouse=True)
def close_dialogs():
    """After each test, close any leftover dialogs so they don't leak into the next test.

    Uses Win32 WM_CLOSE instead of pywinauto — pywinauto's UIA backend
    blocks when a modal dialog is open, making cleanup impossible.

    Loops 3× because the app shows a confirmation QMessageBox AFTER a file
    dialog closes (e.g. "Open Folder" popup after Select Folder), and that
    popup can appear with a delay. One pass would miss it.
    """
    yield
    for _ in range(3):
        time.sleep(0.3)
        close_windows_by_title("About", "Save", "Save File", "Open Folder", "Developer Tools")
    time.sleep(0.3)
