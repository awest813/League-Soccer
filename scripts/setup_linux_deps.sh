#!/usr/bin/env bash
# scripts/setup_linux_deps.sh – install the build dependencies for
# League-Soccer / Gameplay Football on Linux.
#
# Detects your distribution and uses the right package manager:
#   apt (Debian/Ubuntu/Mint), dnf/yum (Fedora/RHEL), pacman (Arch),
#   zypper (openSUSE), xbps (Void).
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

# ── Detect the distribution / package manager ────────────────────────────────
detect_pm() {
  if command -v apt-get >/dev/null 2>&1; then
    echo apt
  elif command -v dnf >/dev/null 2>&1; then
    echo dnf
  elif command -v yum >/dev/null 2>&1; then
    echo yum
  elif command -v pacman >/dev/null 2>&1; then
    echo pacman
  elif command -v zypper >/dev/null 2>&1; then
    echo zypper
  elif command -v xbps-install >/dev/null 2>&1; then
    echo xbps
  else
    echo unknown
  fi
}

PM="$(detect_pm)"

if [[ "${PM}" == "unknown" ]]; then
  echo "ERROR: could not find a supported package manager" >&2
  echo "(apt, dnf/yum, pacman, zypper, or xbps). Please install the build" >&2
  echo "dependencies manually — see README 'Building from Source'." >&2
  exit 1
fi

# install <pkg...> — invokes the detected package manager
install() {
  case "${PM}" in
    apt)
      "${SUDO[@]}" apt-get update -qq
      "${SUDO[@]}" apt-get install -y --no-install-recommends "$@"
      ;;
    dnf|yum)
      "${SUDO[@]}" "${PM}" install -y "$@"
      ;;
    pacman)
      # pacman needs --no-confirm and the -- separator is harmless
      "${SUDO[@]}" pacman -S --noconfirm --needed "$@"
      ;;
    zypper)
      "${SUDO[@]}" zypper --non-interactive install "$@"
      ;;
    xbps)
      # -S refreshes the repository index, -y answers yes to prompts.
      "${SUDO[@]}" xbps-install -Sy "$@"
      ;;
    *)
      echo "ERROR: could not find a supported package manager" >&2
      echo "(apt, dnf/yum, pacman, zypper, or xbps). Please install the" >&2
      echo "build dependencies manually — see README 'Building from Source'." >&2
      exit 1
      ;;
  esac
}

# ── Build the package list for the detected distro ───────────────────────────
# Each distro names the same libraries differently. These arrays hold the
# canonical package names verified against each family's repositories.
case "${PM}" in
  apt)
    CORE_PKGS=(cmake build-essential libboost-dev libsqlite3-dev)
    GAME_PKGS=(libgl1-mesa-dev libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev \
               libsdl2-gfx-dev libopenal-dev xvfb)
    TOOLS_PKGS=(ninja-build clang clang-format clang-tidy doxygen graphviz)
    ;;
  dnf|yum)
    CORE_PKGS=(cmake gcc-c++ make boost-devel sqlite-devel)
    GAME_PKGS=(mesa-libGL-devel SDL2-devel SDL2_image-devel SDL2_ttf-devel \
               SDL2_gfx-devel openal-soft-devel xorg-x11-server-Xvfb)
    TOOLS_PKGS=(ninja-build clang clang-tools-extra doxygen graphviz)
    ;;
  pacman)
    CORE_PKGS=(cmake base-devel boost sqlite)
    GAME_PKGS=(mesa sdl2 sdl2_image sdl2_ttf sdl2_gfx openal xorg-server-xvfb)
    TOOLS_PKGS=(ninja clang clang-tools-extra doxygen graphviz)
    ;;
  zypper)
    CORE_PKGS=(cmake gcc-c++ make libboost_headers-devel sqlite3-devel)
    GAME_PKGS=(Mesa-libGL-devel libSDL2-devel libSDL2_image-devel \
               libSDL2_ttf-devel libSDL2_gfx-devel libopenal-devel xorg-x11-Xvfb)
    TOOLS_PKGS=(ninja clang clang-tools-extra doxygen graphviz)
    ;;
  xbps)
    CORE_PKGS=(cmake base-devel boost-devel sqlite-devel pkg-config)
    GAME_PKGS=(SDL2-devel SDL2_image-devel SDL2_ttf-devel SDL2_gfx-devel \
               MesaLib-devel libopenal-devel xorg-server-xvfb)
    TOOLS_PKGS=(ninja clang clang-tools-extra doxygen graphviz)
    ;;
  *)
    # detect_pm already printed nothing; the install() fallback handles the
    # error, but be explicit here too.
    ;;
esac

PKGS=("${CORE_PKGS[@]}")

if [[ "${MINIMAL}" != true ]]; then
  PKGS+=("${GAME_PKGS[@]}")
fi

if [[ "${WITH_TOOLS}" == true ]]; then
  PKGS+=("${TOOLS_PKGS[@]}")
fi

echo "Detected package manager: ${PM}"
echo "Installing: ${PKGS[*]}"
install "${PKGS[@]}"

echo "Linux build dependencies installed."
