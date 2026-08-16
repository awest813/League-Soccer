#!/usr/bin/env bash
# scripts/setup_macos_deps.sh – install the build dependencies for
# League-Soccer / Gameplay Football on macOS using Homebrew.
#
# Usage:
#   scripts/setup_macos_deps.sh [--minimal] [--with-tools] [--help]
#
#   --minimal      Boost + SQLite only (headless unit tests, no game binary)
#   --with-tools   Also ninja, clang-format, doxygen, graphviz
#   (default)      Full game build dependencies including SDL2/OpenAL

set -euo pipefail

MINIMAL=false
WITH_TOOLS=false

usage() {
  sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'
  exit 0
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --minimal)    MINIMAL=true; shift ;;
    --with-tools) WITH_TOOLS=true; shift ;;
    --help|-h)    usage ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

# Detect Homebrew
BREW_CMD=""
if command -v brew >/dev/null 2>&1; then
  BREW_CMD="brew"
elif [[ -x /opt/homebrew/bin/brew ]]; then
  BREW_CMD="/opt/homebrew/bin/brew"
elif [[ -x /usr/local/bin/brew ]]; then
  BREW_CMD="/usr/local/bin/brew"
fi

if [[ -z "${BREW_CMD}" ]]; then
  echo "ERROR: Homebrew not found." >&2
  echo "macOS dependencies are managed via Homebrew." >&2
  echo "To install Homebrew, run the following command in your terminal:" >&2
  echo '  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"' >&2
  exit 1
fi

echo "Detected Homebrew at: ${BREW_CMD}"

# Package definitions
CORE_PKGS=(cmake ninja boost sqlite)
GAME_PKGS=(sdl2 sdl2_image sdl2_ttf sdl2_gfx openal-soft)
TOOLS_PKGS=(clang-format doxygen graphviz)

PKGS=("${CORE_PKGS[@]}")

if [[ "${MINIMAL}" != true ]]; then
  PKGS+=("${GAME_PKGS[@]}")
fi

if [[ "${WITH_TOOLS}" == true ]]; then
  PKGS+=("${TOOLS_PKGS[@]}")
fi

echo "Installing packages: ${PKGS[*]}"
"${BREW_CMD}" update
"${BREW_CMD}" install "${PKGS[@]}"

echo "macOS build dependencies installed."
