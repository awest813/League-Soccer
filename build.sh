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

# Pick a safe default number of parallel compile jobs when --jobs was not
# given. Each compiler process can use ~1-3 GB of RAM on the Boost-heavy
# translation units in this project, so naively using `nproc` can exhaust
# memory and hang the machine on low-RAM laptops. Allow ~3 GB per job based on
# the system's total physical RAM, and never exceed the CPU count.
#   8 GB  -> 2 jobs   16 GB -> 5 jobs   64 GB -> 21 jobs
default_jobs() {
  local cpus=4 mem_kb=0 jobs
  if command -v nproc >/dev/null 2>&1; then
    cpus="$(nproc)"
  fi
  if [[ -r /proc/meminfo ]]; then
    mem_kb="$(awk '/^MemTotal:/ {print $2}' /proc/meminfo 2>/dev/null || echo 0)"
  elif command -v sysctl >/dev/null 2>&1; then
    mem_kb="$(( $(sysctl -n hw.memsize 2>/dev/null || echo 0) / 1024 ))"
  fi
  local mem_mb=$(( mem_kb / 1024 ))   # convert KiB -> MiB
  if (( mem_mb >= 3072 )); then
    jobs="$(( mem_mb / 3072 ))"       # ~3 GiB per job
  else
    jobs=1
  fi
  (( jobs > cpus )) && jobs="$cpus"
  (( jobs < 1 )) && jobs=1
  echo "$jobs"
}

if [[ -z "${JOBS}" ]]; then
  JOBS="$(default_jobs)"
  echo "==> No --jobs given; using ${JOBS} parallel job(s) (memory-aware)."
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
