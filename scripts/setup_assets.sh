#!/usr/bin/env bash
# scripts/setup_assets.sh – copy (or symlink) the data/ directory into the
# build directory so the game executable can find its runtime assets.
#
# Usage:
#   scripts/setup_assets.sh [--build-dir <path>] [--symlink] [--force] [--help]
#
# Options:
#   --build-dir <path>   Target build directory.  Default: ./build
#   --symlink            Create a symbolic link instead of copying files.
#                        Useful during development so asset edits are
#                        immediately reflected without re-running the script.
#   --force              Remove and replace an existing data/ installation.
#   --help               Show this message and exit.
#
# The script is safe to re-run; it will not overwrite an already-present
# data/ symlink or directory in the build folder unless --force is passed.
#
# Example (typical development workflow):
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build --parallel
#   scripts/setup_assets.sh --build-dir build --symlink
#   ./build/gameplayfootball

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
DATA_DST="${BUILD_DIR}/data"

# Resolve BUILD_DIR relative to the caller's working directory
if [[ "${BUILD_DIR}" != /* ]]; then
  BUILD_DIR="${PWD}/${BUILD_DIR}"
fi
DATA_DST="${BUILD_DIR}/data"

# ── pre-flight checks ─────────────────────────────────────────────────────────
if [[ ! -d "${DATA_SRC}" ]]; then
  echo "Error: source asset directory not found: ${DATA_SRC}" >&2
  exit 1
fi

if [[ ! -d "${BUILD_DIR}" ]]; then
  echo "Build directory does not exist: ${BUILD_DIR}"
  echo "Create it first with:  cmake -S . -B ${BUILD_DIR}"
  exit 1
fi

# ── detect existing installation ──────────────────────────────────────────────
if [[ -e "${DATA_DST}" || -L "${DATA_DST}" ]]; then
  if [[ "${FORCE}" == true ]]; then
    echo "Removing existing: ${DATA_DST}"
    rm -rf "${DATA_DST}"
  else
    echo "Assets already present at: ${DATA_DST}"
    echo "Pass --force to overwrite."
    exit 0
  fi
fi

# ── install ───────────────────────────────────────────────────────────────────
if [[ "${USE_SYMLINK}" == true ]]; then
  echo "Creating symlink: ${DATA_DST} -> ${DATA_SRC}"
  ln -s "${DATA_SRC}" "${DATA_DST}"
  echo "Done.  Asset changes in data/ will be visible immediately."
else
  echo "Copying assets to: ${DATA_DST}"
  cp -R "${DATA_SRC}" "${DATA_DST}"
  echo "Done.  $(find "${DATA_DST}" -type f | wc -l) files copied."
fi

echo ""
echo "You can now run the game from the build directory:"
echo "  cd ${BUILD_DIR} && ./gameplayfootball"
