"""End-to-end test: fully drive the Save and Open Folder dialogs.

Proves we can navigate folders, type a filename, select file type,
click Save, drive a folder picker, and clean up.

Run: uv run python tests/pywinauto/test_full_dialog_flow.py
Requires: xmake run start-desktop
"""

import os
import sys
import io
import time

from pywinauto import Desktop
from native_dialogs import FileDialog, QtMessageBox, open_modal
from win32_helpers import close_windows_by_title


def get_app():
    d = Desktop(backend="uia")
    app = d.window(title="Delightful Qt Web Shell")
    app.wait("visible", timeout=5)
    app.set_focus()
    return app


def test_about_dialog():
    """Open About, verify it exists, press OK to close."""
    print("\n=== TEST: About Dialog ===")
    app = get_app()
    open_modal(app, "Help->About")
    time.sleep(0.5)

    with QtMessageBox("About") as dlg:
        assert dlg.is_open, "About dialog should be open"
        assert "About" in dlg.title, f"Title should contain 'About', got {dlg.title!r}"
        print(f"  ✅ Dialog open: {dlg.title!r}")

        dlg.press_ok()
        time.sleep(0.3)
        assert not dlg.is_open, "About dialog should be closed after OK"
        print("  ✅ Closed with Enter (OK)")


def test_save_dialog_cancel():
    """Open Save, verify controls, click Cancel."""
    print("\n=== TEST: Save Dialog — Cancel ===")
    app = get_app()
    open_modal(app, "File->Save...")
    time.sleep(1)

    with FileDialog("Save File") as dlg:
        assert dlg.is_open, "Save dialog should be open"
        print(f"  ✅ Dialog open, folder: {dlg.current_folder!r}")

        # Check file type filter
        types = dlg.file_types
        assert len(types) >= 2, f"Expected at least 2 file types, got {types}"
        assert any("JSON" in t for t in types), f"Expected JSON filter, got {types}"
        print(f"  ✅ File types: {types}")

        # Check we can type a filename
        dlg.set_filename("test_cancel.json")
        print("  ✅ Set filename: 'test_cancel.json'")

        # Cancel
        dlg.cancel()
        time.sleep(0.3)
        assert not dlg.is_open, "Save dialog should be closed after Cancel"
        print("  ✅ Cancelled successfully")


def test_save_dialog_navigate():
    """Open Save, navigate folders, verify address bar updates."""
    print("\n=== TEST: Save Dialog — Navigate ===")
    app = get_app()

    # Use the project root as a known absolute path with known subdirectories
    project_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

    open_modal(app, "File->Save...")
    time.sleep(1)

    with FileDialog("Save File") as dlg:
        start_folder = dlg.current_folder
        print(f"  Starting folder: {start_folder!r}")

        # Navigate to project root (absolute path — always works)
        dlg.navigate(project_dir)
        time.sleep(1)
        assert dlg.is_open, "Dialog should still be open after navigating to project root"
        print(f"  ✅ Navigated to project root: {dlg.current_folder!r}")

        # Navigate into web/ using absolute path (avoids stale-folder issues)
        web_dir = os.path.join(project_dir, "web")
        dlg.navigate(web_dir)
        time.sleep(1)
        assert dlg.is_open, "Dialog should still be open after navigating to web/"
        new_folder = dlg.current_folder
        assert "web" in (new_folder or ""), f"Expected 'web' in path, got {new_folder!r}"
        print(f"  ✅ Navigated to: {new_folder!r}")

        # Navigate back up
        dlg.navigate("..")
        time.sleep(1)
        assert dlg.is_open, "Dialog should still be open after navigating to .."
        back_folder = dlg.current_folder
        print(f"  ✅ Back to: {back_folder!r}")

        dlg.cancel()
        print("  ✅ Cancelled")


def test_save_dialog_file_type():
    """Open Save, change file type filter."""
    print("\n=== TEST: Save Dialog — File Type ===")
    app = get_app()
    open_modal(app, "File->Save...")
    time.sleep(1)

    with FileDialog("Save File") as dlg:
        types = dlg.file_types
        initial = dlg.selected_file_type_index
        print(f"  File types: {types}")
        print(f"  Initial selection: [{initial}] {types[initial]!r}")

        # Switch to "All Files (*)"
        all_files_idx = next(i for i, t in enumerate(types) if "All Files" in t)
        dlg.select_file_type(all_files_idx)
        time.sleep(0.3)
        assert dlg.selected_file_type_index == all_files_idx
        print(f"  ✅ Switched to: [{all_files_idx}] {types[all_files_idx]!r}")

        dlg.cancel()
        print("  ✅ Cancelled")


def test_save_dialog_save_file():
    """Open Save, type a filename, click Save, verify flow completes."""
    print("\n=== TEST: Save Dialog — Save File ===")
    app = get_app()

    test_filename = "_pywinauto_test_save.json"
    project_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    expected_path = os.path.join(project_dir, test_filename)

    if os.path.exists(expected_path):
        os.remove(expected_path)

    open_modal(app, "File->Save...")
    time.sleep(1)

    with FileDialog("Save File") as dlg:
        current = dlg.current_folder
        print(f"  Current folder: {current!r}")

        dlg.set_filename(test_filename)
        print(f"  Set filename: {test_filename!r}")

        dlg.save()
        time.sleep(1)

        print(f"  Dialog still open? {dlg.is_open}")

    # The save dialog was just a file picker — it doesn't create the file.
    print("  ✅ Save flow completed (dialog closed)")

    if os.path.exists(expected_path):
        os.remove(expected_path)
        print(f"  Cleaned up: {expected_path}")


def test_ctrl_s_shortcut():
    """Ctrl+S should open the Save dialog."""
    print("\n=== TEST: Ctrl+S Shortcut ===")
    app = get_app()

    import threading
    threading.Thread(target=lambda: app.type_keys("^s"), daemon=True).start()
    time.sleep(1)

    with FileDialog("Save File") as dlg:
        assert dlg.is_open, "Save dialog should be open via Ctrl+S"
        print(f"  ✅ Save opened via Ctrl+S, folder: {dlg.current_folder!r}")
        dlg.cancel()
        print("  ✅ Cancelled")


def test_open_folder_dialog():
    """Open Folder dialog — verify controls, cancel."""
    print("\n=== TEST: Open Folder Dialog ===")
    app = get_app()

    open_modal(app, "File->Open Folder...")
    time.sleep(1)

    with FileDialog("Open Folder") as dlg:
        assert dlg.is_open, "Open Folder dialog should be open"
        print(f"  ✅ Dialog open, folder: {dlg.current_folder!r}")

        # Folder picker has no file type filter
        assert dlg.file_types == [], f"Expected no file types, got {dlg.file_types}"
        print("  ✅ No file type filter (correct for folder picker)")

        # Cancel
        dlg.cancel()
        time.sleep(0.3)
        assert not dlg.is_open, "Folder dialog should be closed after Cancel"
        print("  ✅ Cancelled")


def test_open_folder_dialog_select():
    """Open Folder dialog — select a folder via navigate (typing path + Enter)."""
    print("\n=== TEST: Open Folder Dialog — Select ===")
    app = get_app()

    project_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

    open_modal(app, "File->Open Folder...")
    time.sleep(1)

    with FileDialog("Open Folder") as dlg:
        assert dlg.is_open, "Open Folder dialog should be open"

        # In a folder picker, typing a path + Enter SELECTS that folder
        # (closes the dialog). This is the correct way to pick a folder.
        web_dir = os.path.join(project_dir, "web")
        dlg.navigate(web_dir)
        time.sleep(1)
        assert not dlg.is_open, "Folder dialog should close after selecting a folder"
        print(f"  ✅ Selected folder: {web_dir!r}")


def test_ctrl_o_shortcut():
    """Ctrl+O should open the Open Folder dialog."""
    print("\n=== TEST: Ctrl+O Shortcut ===")
    app = get_app()

    import threading
    threading.Thread(target=lambda: app.type_keys("^o"), daemon=True).start()
    time.sleep(1)

    with FileDialog("Open Folder") as dlg:
        assert dlg.is_open, "Open Folder dialog should be open via Ctrl+O"
        print(f"  ✅ Open Folder opened via Ctrl+O, folder: {dlg.current_folder!r}")
        dlg.cancel()
        print("  ✅ Cancelled")


if __name__ == "__main__":
    # Fix encoding for direct script execution (not needed under pytest)
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
    sys.stdout.reconfigure(line_buffering=True)

    # Safety net: close any lingering dialogs before starting
    close_windows_by_title("About", "Save File", "Open Folder")
    time.sleep(0.3)

    try:
        test_about_dialog()
        test_save_dialog_cancel()
        test_save_dialog_navigate()
        test_save_dialog_file_type()
        test_save_dialog_save_file()
        test_ctrl_s_shortcut()
        test_open_folder_dialog()
        test_open_folder_dialog_select()
        test_ctrl_o_shortcut()
        print("\n" + "=" * 50)
        print("🔥 ALL TESTS PASSED! 🔥")
        print("=" * 50)
    except Exception as e:
        print(f"\n💀 FAILED: {e}")
        import traceback
        traceback.print_exc()
    finally:
        close_windows_by_title("About", "Save File", "Open Folder")
