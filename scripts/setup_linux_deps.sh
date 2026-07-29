#!/usr/bin/env bash
# scripts/setup_linux_deps.sh – install apt packages needed to build on Debian/Ubuntu.
#
# Usage:
#   scripts/setup_linux_deps.sh [--minimal] [--with-tools] [--help]
#
#   --minimal      Boost + SQLite only (headless unit tests, no game binary)
#   --with-tools   Also clang-format, clang-tidy, doxygen, graphviz, ninja
#   (default)      Full game build dependencies including SDL2/OpenGL/OpenAL/Xvfb

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

if [[ "${EUID}" -ne 0 ]]; then
  SUDO=(sudo)
else
  SUDO=()
fi

"${SUDO[@]}" apt-get update -qq

PKGS=(cmake build-essential libboost-dev libsqlite3-dev)

if [[ "${MINIMAL}" != true ]]; then
  PKGS+=(
    libgl1-mesa-dev
    libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-gfx-dev
    libopenal-dev
    xvfb
  )
fi

if [[ "${WITH_TOOLS}" == true ]]; then
  PKGS+=(ninja-build clang clang-format clang-tidy doxygen graphviz)
fi

"${SUDO[@]}" apt-get install -y --no-install-recommends "${PKGS[@]}"
echo "Linux build dependencies installed."
