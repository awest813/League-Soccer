#!/bin/sh
# scripts/ci_void_linux.sh – build and test inside a Void Linux container.
#
# Run as root from the repository root of a ghcr.io/void-linux/void-glibc-full
# or void-musl-full container (CI mounts the checkout there). It installs the
# dependencies with scripts/setup_linux_deps.sh, builds with ./build.sh, runs
# the unit tests and a few smoke runs under Xvfb (Void has no xvfb-run, so
# tests/run_menu_smoke.sh starts Xvfb itself).
#
# Written for POSIX sh: the base image has no bash until it is installed.

set -eu

# xbps refuses to install anything while xbps itself is out of date.
xbps-install -Syu xbps
xbps-install -Syu
xbps-install -y bash

exec bash -euo pipefail -c '
bash scripts/setup_linux_deps.sh

./build.sh --no-deps --debug --test --jobs "$(nproc)"

cd build
echo "==> libc: $(ldd --version 2>&1 | head -n1)"

../tests/run_menu_smoke.sh ./gameplayfootball ../data/menu_smoke_quick_match.config 120 \
  "[menu-smoke] Main menu ready, opening Quick Match" \
  "[menu-smoke] Gameplay page reached and live match is active" \
  "[menu-smoke] Quick Match verification succeeded, quitting test run"

../tests/run_menu_smoke.sh ./gameplayfootball ../data/menu_smoke_league.config 180 \
  "[menu-smoke] League save created, entering league page" \
  "[menu-smoke] League page flow succeeded"

../tests/run_menu_smoke.sh ./gameplayfootball ../data/menu_smoke_career.config 120 \
  "[menu-smoke] Career mode menu reached successfully"

# A whole match exercises the AI, physics and render threads for longest.
../tests/run_menu_smoke.sh ./gameplayfootball ../data/menu_smoke_full_match.config 900 \
  "[menu-smoke] Full match complete:" \
  "[menu-smoke] Full-match verification succeeded, quitting test run"
'
