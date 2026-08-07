#!/usr/bin/env bash
# scripts/setup_linux_deps.sh – install the build dependencies for
# League-Soccer / Gameplay Football on Linux.
#
# Detects your distribution and uses the right package manager:
#   apt (Debian/Ubuntu/Mint/Devuan), dnf/yum (Fedora/RHEL),
#   pacman (Arch/Manjaro), zypper (openSUSE), xbps (Void),
#   apk (Alpine/PostmarketOS), emerge (Gentoo).
# On NixOS it prints a hint pointing at shell.nix / nix-develop instead.
#
# Usage:
#   scripts/setup_linux_deps.sh [--minimal] [--with-tools] [--detect] [--help]
#
#   --minimal      Boost + SQLite only (headless unit tests, no game binary)
#   --with-tools   Also clang-format, clang-tidy, doxygen, graphviz, ninja
#   --detect       Only print the detected package manager and exit
#   (default)      Full game build dependencies including SDL2/OpenGL/OpenAL/Xvfb

set -euo pipefail

MINIMAL=false
WITH_TOOLS=false
DETECT_ONLY=false

usage() {
  sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'
  exit 0
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --minimal)    MINIMAL=true; shift ;;
    --with-tools) WITH_TOOLS=true; shift ;;
    --detect)     DETECT_ONLY=true; shift ;;
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
# Prefer the authoritative distro identity from /etc/os-release, then validate
# the corresponding package manager is actually installed, falling back to a
# `command -v` scan (handles containers/images that lack /etc/os-release).
pm_for_distro() {
  local id
  id="$1"
  case "${id}" in
    debian|ubuntu|linuxmint|devuan|pop|elementary|raspbian|kali|trisquel|neon)
      echo apt ;;
    fedora|rhel|centos|rocky|almalinux|ol|amzn|cirros)
      echo dnf ;;
    arch|manjaro|endeavouros|artix|garuda|cachyos|arcolinux)
      echo pacman ;;
    opensuse|opensuse-leap|opensuse-tumbleweed|suse|sles)
      echo zypper ;;
    void)
      echo xbps ;;
    alpine|postmarketos)
      echo apk ;;
    gentoo|funtoo|calculate)
      echo emerge ;;
    nixos)
      echo nix ;;
  esac
}

pm_bin() {
  case "$1" in
    apt)     echo apt-get ;;
    dnf)     echo dnf ;;
    yum)     echo yum ;;
    pacman)  echo pacman ;;
    zypper)  echo zypper ;;
    xbps)    echo xbps-install ;;
    apk)     echo apk ;;
    emerge)  echo emerge ;;
    nix)     echo nix ;;
    *)       echo "" ;;
  esac
}

detect_pm() {
  local pm=""
  if [[ -f /etc/os-release && -r /etc/os-release ]]; then
    # shellcheck disable=SC1091
    source /etc/os-release
    local id="${ID:-}" id_like="${ID_LIKE:-}"
    for cand in "${id}" ${id_like}; do
      pm="$(pm_for_distro "${cand}")"
      [[ -z "${pm}" ]] && continue
      if command -v "$(pm_bin "${pm}")" >/dev/null 2>&1; then
        echo "${pm}"
        return 0
      fi
    done
  fi
  # Fallback: scan for a known package manager binary.
  for cand in \
      apt-get:apt \
      dnf:dnf \
      yum:yum \
      pacman:pacman \
      zypper:zypper \
      xbps-install:xbps \
      apk:apk \
      emerge:emerge \
      nix:nix; do
    local bin="${cand%%:*}" pmc="${cand##*:}"
    if command -v "${bin}" >/dev/null 2>&1; then
      echo "${pmc}"
      return 0
    fi
  done
  echo unknown
}

PM="$(detect_pm)"

if [[ "${DETECT_ONLY}" == true ]]; then
  echo "${PM}"
  exit 0
fi

if [[ "${PM}" == "nix" ]]; then
  echo "Detected NixOS."
  echo "This script installs system packages, which does not fit Nix's"
  echo "declarative model. Instead, enter a dev-shell with:"
  echo "    nix develop   # or:  nix-shell"
  echo "and then build with: ./build.sh --no-deps"
  exit 0
fi

if [[ "${PM}" == "unknown" ]]; then
  echo "ERROR: could not find a supported package manager" >&2
  echo "(apt, dnf/yum, pacman, zypper, xbps, apk, or emerge). Please install" >&2
  echo "the dependencies manually — see README 'Building from Source'." >&2
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
      # pacman needs --noconfirm and --needed to skip already-installed pkgs
      "${SUDO[@]}" pacman -S --noconfirm --needed "$@"
      ;;
    zypper)
      "${SUDO[@]}" zypper --non-interactive install "$@"
      ;;
    xbps)
      # -S refreshes the repository index, -y answers yes to prompts.
      "${SUDO[@]}" xbps-install -Sy "$@"
      ;;
    apk)
      "${SUDO[@]}" apk add --no-cache "$@"
      ;;
    emerge)
      # --ask=n is non-interactive, --noreplace skips already-installed atoms.
      "${SUDO[@]}" emerge --ask=n --noreplace "$@"
      ;;
    *)
      echo "ERROR: could not find a supported package manager" >&2
      echo "(apt, dnf/yum, pacman, zypper, xbps, apk, or emerge). Please" >&2
      echo "install the dependencies manually — see README 'Building from Source'." >&2
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
    GAME_PKGS=(Mesa-libGL-devel SDL2-devel SDL2_image-devel \
               SDL2_ttf-devel SDL2_gfx-devel openal-soft-devel xorg-x11-server-Xvfb)
    TOOLS_PKGS=(ninja clang clang-tools-extra doxygen graphviz)
    ;;
  xbps)
    CORE_PKGS=(cmake base-devel boost-devel sqlite-devel pkg-config)
    GAME_PKGS=(SDL2-devel SDL2_image-devel SDL2_ttf-devel SDL2_gfx-devel \
               MesaLib-devel libopenal-devel xorg-server-xvfb)
    TOOLS_PKGS=(ninja clang clang-tools-extra doxygen graphviz)
    ;;
  apk)
    CORE_PKGS=(cmake build-base boost-dev sqlite-dev pkgconf)
    GAME_PKGS=(mesa-dev sdl2-dev sdl2_image-dev sdl2_ttf-dev sdl2_gfx-dev \
               openal-soft-dev xvfb)
    TOOLS_PKGS=(ninja clang clang-extra-tools doxygen graphviz)
    ;;
  emerge)
    CORE_PKGS=(dev-build/cmake dev-build/ninja dev-libs/boost dev-db/sqlite)
    GAME_PKGS=(media-libs/mesa media-libs/libsdl2 media-libs/sdl2-image \
               media-libs/sdl2-ttf media-libs/sdl2-gfx media-libs/openal \
               x11-base/xorg-server[xvfb])
    TOOLS_PKGS=(llvm-core/clang[extra] app-doc/doxygen media-gfx/graphviz)
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
