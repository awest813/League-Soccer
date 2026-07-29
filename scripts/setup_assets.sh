#!/usr/bin/env bash
# scripts/setup_assets.sh – install runtime assets next to the game executable.
#
# The game looks for paths relative to the process working directory:
#   football.config, media/, databases/, locale/
# CMake POST_BUILD already copies these beside gameplayfootball.  Use this
# script when you need a manual install (Docker, packaging, symlink workflow).
#
# Usage:
#   scripts/setup_assets.sh [--build-dir <path>] [--symlink] [--force] [--help]
#
# Options:
#   --build-dir <path>   Target directory (where the executable lives).
#                        Default: ./build
#   --symlink            Symlink media/databases/locale instead of copying.
#                        Useful in development so asset edits apply immediately.
#   --force              Remove and replace existing asset installs.
#   --help               Show this message and exit.
#
# Example:
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build --parallel
#   scripts/setup_assets.sh --build-dir build --symlink --force
#   (cd build && ./gameplayfootball)

set -euo pipefail

# ── defaults ──────────────────────────────────────────────────────────────────
BUILD_DIR="build"
USE_SYMLINK=false
FORCE=false

# ── argument parsing ──────────────────────────────────────────────────────────
usage() {
  sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'
  exit 0
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir)   BUILD_DIR="$2"; shift 2 ;;
    --symlink)     USE_SYMLINK=true; shift ;;
    --force)       FORCE=true; shift ;;
    --help|-h)     usage ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

# ── resolve paths ─────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DATA_SRC="${REPO_ROOT}/data"

if [[ "${BUILD_DIR}" != /* ]]; then
  BUILD_DIR="${PWD}/${BUILD_DIR}"
fi

# ── pre-flight checks ─────────────────────────────────────────────────────────
if [[ ! -d "${DATA_SRC}" ]]; then
  echo "Error: source asset directory not found: ${DATA_SRC}" >&2
  exit 1
fi

if [[ ! -d "${BUILD_DIR}" ]]; then
  echo "Build directory does not exist: ${BUILD_DIR}"
  echo "Create it first with:  cmake -S . -B <build-dir>"
  exit 1
fi

install_path() {
  local name="$1"
  local src="$2"
  local dst="$3"

  if [[ -e "${dst}" || -L "${dst}" ]]; then
    if [[ "${FORCE}" == true ]]; then
      rm -rf "${dst}"
    else
      echo "Already present: ${dst} (pass --force to replace)"
      return 0
    fi
  fi

  mkdir -p "$(dirname "${dst}")"
  if [[ "${USE_SYMLINK}" == true && -d "${src}" ]]; then
    echo "Symlink ${name}: ${dst} -> ${src}"
    ln -s "${src}" "${dst}"
  elif [[ -d "${src}" ]]; then
    echo "Copy ${name}: ${dst}"
    cp -R "${src}" "${dst}"
  else
    echo "Copy ${name}: ${dst}"
    cp -f "${src}" "${dst}"
  fi
}

# Layout mirrors CMakeLists.txt POST_BUILD for gameplayfootball.
install_path "football.config" "${DATA_SRC}/football.config" "${BUILD_DIR}/football.config"
install_path "media"           "${DATA_SRC}/media"           "${BUILD_DIR}/media"
install_path "databases"       "${DATA_SRC}/databases"       "${BUILD_DIR}/databases"
install_path "locale"          "${DATA_SRC}/locale"          "${BUILD_DIR}/locale"

# Compatibility copies under data/ (some loaders and tools expect both).
mkdir -p "${BUILD_DIR}/data"
install_path "data/football.config" "${DATA_SRC}/football.config" "${BUILD_DIR}/data/football.config"
install_path "data/locale"          "${DATA_SRC}/locale"          "${BUILD_DIR}/data/locale"

echo ""
echo "Assets ready. Run the game from the build directory:"
echo "  cd ${BUILD_DIR} && ./gameplayfootball"
