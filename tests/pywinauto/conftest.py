"""Shared fixtures for pywinauto tests.

These tests require the desktop app to be running:
    xmake run start-desktop

Run with:
    uv run pytest tests/pywinauto/
"""

import time

import pytest
from pywinauto import Desktop

from win32_helpers import close_windows_by_title


APP_TITLE = "Delightful Qt Web Shell"
APP_CLASS = "QMainWindow"


@pytest.fixture
def desktop():
    return Desktop(backend="uia")


@pytest.fixture
def app(desktop):
    """Find the running Qt app window. Fails if the app isn't running."""
    try:
        window = desktop.window(title=APP_TITLE, class_name=APP_CLASS)
        window.wait("visible", timeout=5)
        return window
    except Exception:
        pytest.fail(
            f"App not found. Is it running? Start it with: xmake run start-desktop"
        )


@pytest.fixture(autouse=True)
def close_dialogs():
    """After each test, close any leftover dialogs so they don't leak into the next test.

    Uses Win32 WM_CLOSE instead of pywinauto — pywinauto's UIA backend
    blocks when a modal dialog is open, making cleanup impossible.
    """
    yield
    time.sleep(0.3)
    close_windows_by_title("About", "Export Data", "Save", "Open", "Developer Tools")
    time.sleep(0.3)
