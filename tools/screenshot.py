"""Capture desktop screenshots for agent-driven testing and debugging.

Agents can't see the screen. This tool gives them eyes on the full desktop —
not just the web content (CDP screenshot does that), but native dialogs,
menus, taskbar, everything.

Usage:
    uv run python tools/screenshot.py                    # primary monitor → screenshot.png
    uv run python tools/screenshot.py --all              # all monitors → screenshot_all.png
    uv run python tools/screenshot.py --monitor 2        # specific monitor
    uv run python tools/screenshot.py --output shot.png  # custom path
    uv run python tools/screenshot.py --list             # list available monitors

The output path is printed to stdout so agents can read the image file.

Disabled by default in CI (set SCREENSHOTS_ENABLED=1 to override).
"""

import argparse
import os
import sys

import mss
import mss.tools


def list_monitors():
    """Print available monitors and their geometry."""
    with mss.mss() as sct:
        for i, mon in enumerate(sct.monitors):
            label = "all monitors" if i == 0 else f"monitor {i}"
            print(f"  [{i}] {label}: {mon['width']}x{mon['height']} at ({mon['left']}, {mon['top']})")


def capture(monitor_index=None, capture_all=False, output=None):
    """Capture a screenshot and return the output path."""
    with mss.mss() as sct:
        if capture_all:
            # monitors[0] is the virtual screen spanning all monitors
            target = sct.monitors[0]
            default_name = "screenshot_all.png"
        elif monitor_index is not None:
            if monitor_index < 1 or monitor_index >= len(sct.monitors):
                print(f"Monitor {monitor_index} not found. Available: 1-{len(sct.monitors) - 1}", file=sys.stderr)
                sys.exit(1)
            target = sct.monitors[monitor_index]
            default_name = f"screenshot_mon{monitor_index}.png"
        else:
            # Primary monitor (index 1)
            target = sct.monitors[1] if len(sct.monitors) > 1 else sct.monitors[0]
            default_name = "screenshot.png"

        out_path = output or default_name
        img = sct.grab(target)
        mss.tools.to_png(img.rgb, img.size, output=out_path)
        return os.path.abspath(out_path)


def main():
    parser = argparse.ArgumentParser(description="Capture desktop screenshots")
    parser.add_argument("--monitor", "-m", type=int, help="Monitor number (1-based). Default: primary.")
    parser.add_argument("--all", "-a", action="store_true", help="Capture all monitors as one image.")
    parser.add_argument("--output", "-o", help="Output file path. Default: screenshot.png")
    parser.add_argument("--list", "-l", action="store_true", help="List available monitors and exit.")
    args = parser.parse_args()

    # Safety: disabled in CI by default
    if os.environ.get("CI") and not os.environ.get("SCREENSHOTS_ENABLED"):
        print("Screenshots disabled in CI. Set SCREENSHOTS_ENABLED=1 to enable.", file=sys.stderr)
        sys.exit(0)

    if args.list:
        list_monitors()
        return

    path = capture(
        monitor_index=args.monitor,
        capture_all=args.all,
        output=args.output,
    )
    # Print the path so agents can read the file
    print(path)


if __name__ == "__main__":
    main()
