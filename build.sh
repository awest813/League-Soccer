#!/usr/bin/env bash
# build.sh – one-shot build for League-Soccer / Gameplay Football on Linux.
#
# Installs dependencies (if needed), configures CMake, and builds the game.
# Runtime assets (media/, databases/, locale/, football.config) are copied
# next to the binary automatically by the CMake POST_BUILD step.
#
# Usage:
#   ./build.sh [options]
#
# Options:
#   --release    Optimised build (default)
#   --debug      Debug build with assertions
#   --clean      Remove build/ before building
#   --no-deps    Skip the dependency-install step (use the system libraries)
#   --jobs N     Parallel compile jobs (default: number of CPUs)
#   --help, -h   Show this message
#
# Examples:
#   ./build.sh                  # release build, installs deps first
#   ./build.sh --debug --clean  # clean debug build
#   ./build.sh --no-deps        # rebuild using already-installed deps

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE=Release
CLEAN=false
INSTALL_DEPS=true
JOBS=""

usage() {
  sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'
  exit 0
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --release)  BUILD_TYPE=Release; shift ;;
    --debug)    BUILD_TYPE=Debug; shift ;;
    --clean)    CLEAN=true; shift ;;
    --no-deps)  INSTALL_DEPS=false; shift ;;
    --jobs)     JOBS="$2"; shift 2 ;;
    --help|-h)  usage ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

# Pick a sensible default parallelism if --jobs was not given.
if [[ -z "${JOBS}" ]]; then
  if command -v nproc >/dev/null 2>&1; then
    JOBS="$(nproc)"
  else
    JOBS=4
  fi
fi

cd "${SCRIPT_DIR}"

# ── 1. Dependencies ──────────────────────────────────────────────────────────
if [[ "${INSTALL_DEPS}" == true ]]; then
  echo "==> Installing build dependencies…"
  bash scripts/setup_linux_deps.sh
fi

# ── 2. Clean ─────────────────────────────────────────────────────────────────
if [[ "${CLEAN}" == true ]]; then
  echo "==> Removing build/ …"
  rm -rf build/
fi

# ── 3. Configure ─────────────────────────────────────────────────────────────
echo "==> Configuring (${BUILD_TYPE})…"
cmake -S . -B build -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

# ── 4. Build ─────────────────────────────────────────────────────────────────
echo "==> Building with ${JOBS} parallel job(s)…"
cmake --build build --parallel "${JOBS}"

echo
echo "Build complete. Run the game with:  ./run.sh"
echo "(or:  cd build && ./gameplayfootball)"
