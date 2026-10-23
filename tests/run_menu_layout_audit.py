"""Launch standalone menus and reject out-of-screen labels/controls.

Usage: python tests/run_menu_layout_audit.py build-win/Release/gameplayfootball.exe
Uses temporary configs; no user settings are overwritten.
"""
import argparse
from contextlib import closing
import os
from pathlib import Path
import subprocess
import sqlite3
import tempfile

ROUTES = "widgets settings gameplay controller keyboard gamepads gamepad_setup gamepad_setup_virtual gamepad_calibration gamepad_mapping gamepad_function graphics audio language credits match_options forfeit history career career_new career_save career_training career_owner_gm career_owner_legacy career_owner_delegated career_coach career_player career_player_missing career_player_training career_season career_season_pending career_matchday career_bid_detail career_standings career_tactics career_tactics_delegated career_press career_roster career_roster_player".split()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", type=Path)
    parser.add_argument("--language", default="en")
    parser.add_argument("--width", type=int, default=1280)
    parser.add_argument("--height", type=int, default=720)
    parser.add_argument("--routes", nargs="+", default=ROUTES)
    parser.add_argument("--hub-section", type=int, choices=range(5), default=0)
    args = parser.parse_args()
    exe = args.executable.resolve()
    failures = []
    for route in args.routes:
        with tempfile.TemporaryDirectory(prefix="menu-audit-", dir=exe.parent) as temp:
            config = Path(temp) / "audit.config"
            config.write_text(f'"debug" "true"\n"quick_start" "false"\n"menu_layout_audit" "true"\n"menu_smoke_test_page" "{route}"\n"locale_language" "{args.language}"\n"context_x" "{args.width}"\n"context_y" "{args.height}"\n"context_fullscreen" "false"\n', encoding="utf-8")
            with config.open("a", encoding="utf-8") as stream:
                stream.write(f'"menu_smoke_career_section" "{args.hub_section}"\n')
                if route.endswith("_virtual"):
                    # SDL virtual joystick: a real HIDGamepad without hardware.
                    stream.write('"menu_smoke_virtual_gamepad" "true"\n')
            if route == "career_training":
                fixture = Path(temp) / "career"
                fixture.mkdir()
                players = [f"player.{i}=Player {i + 1} with a long display name|CM|19|65|85|100000|500|70|60|85|0|0|0|{i * 3}" for i in range(24)]
                players += [f"youth.{i}=Academy prospect {i + 1}|CF|17|55|90|50000|500|70|60|100|0|0|0|25" for i in range(8)]
                (fixture / "career.save").write_text("name=Training Audit\ntrainingPlan=1\n" + "\n".join(players) + "\n", encoding="utf-8")
                with config.open("a", encoding="utf-8") as stream:
                    stream.write(f'"menu_smoke_career_save_directory" "{fixture.as_posix()}"\n')
            if route in ("career_bid_detail", "career_standings", "career_season_pending", "career_tactics", "career_tactics_delegated", "career_press", "career_roster", "career_roster_player", "career_owner_gm", "career_owner_legacy", "career_owner_delegated", "career_coach", "career_player", "career_player_missing", "career_player_training", "career_season", "career_matchday"):
                fixture = Path(temp) / "career"
                fixture.mkdir()
                role = {"career_bid_detail": 4, "career_standings": 3, "career_season_pending": 4, "career_tactics": 3, "career_tactics_delegated": 2, "career_press": 3, "career_roster": 4, "career_roster_player": 0, "career_owner_legacy": 4, "career_owner_delegated": 2, "career_player_missing": 0, "career_player_training": 0, "career_owner_gm": 4, "career_coach": 3, "career_player": 0, "career_season": 4, "career_matchday": 3}[route]
                (fixture / "career.save").write_text(
                    f"name=Career Audit with a long club and career name\nmanagerName=Alex with a long manager name\nmode={role}\nseason=2\nweek={38 if route == 'career_season_pending' else 5}\nmaxWeeks=38\n"
                    + f"controlledEntityID={999 if route == 'career_player_missing' else 100}\n"
                    + "player.0=Alex with a long footballer display name|CM|19|65|85|100000|500|70|60|85|4|6|12|18|0|100|100|1|3|500|0|0|0|0|0|0|CM|0\n"
                    + ("".join(f"player.{i}=Footballer {i} with a long name|CM|20|65|85|100000|500|70|60|85|0|0|0|0|0|{100+i}\n" for i in range(1,24)) if route.startswith("career_roster") else "")
                    + ("target.0=Transfer Target with a long display name|CM|78|86|24|12000000|45000|15000000|2|1\n" if route == "career_bid_detail" else "")
                    + "".join(f"inbox.{i}={i}|0|5|0|0|0|Career message {i} with a long subject to exercise text fitting|Your season update and training report with a long body for message display.\n" for i in range(12)),
                    encoding="utf-8")
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
                if args.hub_section == 4 and route in ("career_owner_gm", "career_owner_legacy", "career_owner_delegated", "career_coach", "career_player", "career_player_missing"):
                    with closing(sqlite3.connect(f"file:{(fixture / 'career.save').as_posix()}?mode=ro", uri=True)) as connection:
                        saved = connection.execute("SELECT data FROM career_payload WHERE id=1").fetchone()[0]
                    if "inbox.0=0|0|5|1|" not in saved:
                        errors.append("Inbox read status was not persisted")
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
