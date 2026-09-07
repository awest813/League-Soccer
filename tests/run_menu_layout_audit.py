"""Launch standalone menus and reject out-of-screen labels/controls.

Usage: python tests/run_menu_layout_audit.py build-win/Release/gameplayfootball.exe
Uses temporary configs; no user settings are overwritten.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROUTES = "widgets settings gameplay controller keyboard gamepads gamepad_setup gamepad_calibration gamepad_mapping gamepad_function graphics audio language credits match_options forfeit history career career_new career_save career_training".split()

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
            if route == "career_training":
                fixture = Path(temp) / "career"
                fixture.mkdir()
                players = [f"player.{i}=Player {i + 1} with a long display name|CM|19|65|85|100000|500|70|60|85|0|0|0|{i * 3}" for i in range(24)]
                players += [f"youth.{i}=Academy prospect {i + 1}|CF|17|55|90|50000|500|70|60|100|0|0|0|25" for i in range(8)]
                (fixture / "career.save").write_text("name=Training Audit\ntrainingPlan=1\n" + "\n".join(players) + "\n", encoding="utf-8")
                with config.open("a", encoding="utf-8") as stream:
                    stream.write(f'"menu_smoke_career_save_directory" "{fixture.as_posix()}"\n')
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
                    diagnostics = [line for line in output.splitlines() if "[menu-smoke]" in line or "[menu-layout]" in line]
                    print("\n".join(errors or diagnostics) if errors or diagnostics else output[-3000:], flush=True)
                else:
                    print(f"PASS {route}", flush=True)
            except subprocess.TimeoutExpired:
                failures.append(route)
                print(f"FAIL {route}: timed out", flush=True)
    print(f"{len(args.routes) - len(failures)}/{len(args.routes)} menu routes passed", flush=True)
    return bool(failures)

if __name__ == "__main__":
    raise SystemExit(main())
