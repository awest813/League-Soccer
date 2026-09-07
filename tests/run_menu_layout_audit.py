"""Launch standalone menus and reject out-of-screen labels/controls.

Usage: python tests/run_menu_layout_audit.py build-win/Release/gameplayfootball.exe
Uses temporary configs; no user settings are overwritten.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROUTES = "widgets settings gameplay controller keyboard gamepads gamepad_setup gamepad_calibration gamepad_mapping gamepad_function graphics audio language credits match_options forfeit history career career_new career_save".split()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", type=Path)
    parser.add_argument("--language", default="en")
    parser.add_argument("--width", type=int, default=1280)
    parser.add_argument("--height", type=int, default=720)
    parser.add_argument("--routes", nargs="+", default=ROUTES)
    args = parser.parse_args()
    exe = args.executable.resolve()
    failures = []
    for route in args.routes:
        with tempfile.TemporaryDirectory(prefix="menu-audit-", dir=exe.parent) as temp:
            config = Path(temp) / "audit.config"
            config.write_text(f'"debug" "true"\n"quick_start" "false"\n"menu_layout_audit" "true"\n"menu_smoke_test_page" "{route}"\n"locale_language" "{args.language}"\n"context_x" "{args.width}"\n"context_y" "{args.height}"\n"context_fullscreen" "false"\n', encoding="utf-8")
            options = {}
            if os.name == "nt":
                startup = subprocess.STARTUPINFO()
                startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
                startup.wShowWindow = subprocess.SW_HIDE
                options["startupinfo"] = startup
            try:
                result = subprocess.run([str(exe), str(config)], cwd=exe.parent,
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                        timeout=30, **options)
                output = result.stdout.decode("utf-8", errors="replace")
                marker = f"[menu-smoke] Standalone {route} reached successfully"
                errors = [line for line in output.splitlines() if "[menu-layout] OUTSIDE" in line]
                if result.returncode or marker not in output or errors:
                    failures.append(route)
                    print(f"FAIL {route}: exit={result.returncode}", flush=True)
                    print("\n".join(errors) if errors else output[-3000:], flush=True)
                else:
                    print(f"PASS {route}", flush=True)
            except subprocess.TimeoutExpired:
                failures.append(route)
                print(f"FAIL {route}: timed out", flush=True)
    print(f"{len(args.routes) - len(failures)}/{len(args.routes)} menu routes passed", flush=True)
    return bool(failures)

if __name__ == "__main__":
    raise SystemExit(main())
