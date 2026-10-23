#!/usr/bin/env bash
# scripts/package_macos.sh – package League-Soccer as a macOS app.
#
# Builds "Gameplay Football.app" in the output directory:
#   Contents/MacOS/gameplayfootball   the game
#   Contents/Resources/               media, databases, locale, football.config
#   Contents/Frameworks/              the Homebrew libraries it needs (SDL2 …)
# Library references are rewritten to @rpath so the app runs on a Mac without
# Homebrew, and the app is signed ad hoc (Apple Silicon refuses to run a
# modified, unsigned binary). The game keeps its settings and saves in
# ~/Library/Application Support/Gameplay Football. With --zip it also writes
# a zip next to the app (made with ditto, which keeps the bundle intact).
#
# Usage:
#   scripts/package_macos.sh --build-dir <path> --out-dir <path> [--strip] [--zip]

set -euo pipefail

BUILD_DIR=""
OUT_DIR=""
STRIP=false
ZIP=false

usage() {
  sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'
  exit 0
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir) BUILD_DIR="$2"; shift 2 ;;
    --out-dir)   OUT_DIR="$2"; shift 2 ;;
    --strip)     STRIP=true; shift ;;
    --zip)       ZIP=true; shift ;;
    --help|-h)   usage ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

if [[ -z "${BUILD_DIR}" || -z "${OUT_DIR}" ]]; then
  echo "Error: --build-dir and --out-dir are required." >&2
  usage
fi
if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "Error: macOS packaging needs otool, install_name_tool and codesign." >&2
  exit 1
fi
if [[ ! -x "${BUILD_DIR}/gameplayfootball" ]]; then
  echo "Error: ${BUILD_DIR}/gameplayfootball not found or not executable." >&2
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
APP_NAME="Gameplay Football"
APP="${OUT_DIR}/${APP_NAME}.app"
MACOS_DIR="${APP}/Contents/MacOS"
RESOURCES_DIR="${APP}/Contents/Resources"
FRAMEWORKS_DIR="${APP}/Contents/Frameworks"
EXE="${MACOS_DIR}/gameplayfootball"

echo "==> Packaging macOS app from ${BUILD_DIR} into ${APP}"
rm -rf "${APP}"
mkdir -p "${MACOS_DIR}" "${RESOURCES_DIR}" "${FRAMEWORKS_DIR}"

cp "${BUILD_DIR}/gameplayfootball" "${EXE}"
chmod 755 "${EXE}"
if [[ "${STRIP}" == true ]]; then
  strip -x "${EXE}" || true
fi

# ── Assets ───────────────────────────────────────────────────────────────────
copy_asset() {  # copy_asset <relative path>; prefer the build tree's copy
  local rel="$1"
  local source="${BUILD_DIR}/${rel}"
  [[ -e "${source}" ]] || source="${REPO_ROOT}/data/${rel#data/}"
  if [[ -e "${source}" ]]; then
    mkdir -p "$(dirname "${RESOURCES_DIR}/${rel}")"
    cp -R "${source}" "${RESOURCES_DIR}/${rel}"
  fi
}
for rel in football.config media databases locale data/football.config data/locale; do
  copy_asset "${rel}"
done
for doc in LICENSE README.md; do
  [[ -f "${REPO_ROOT}/${doc}" ]] && cp "${REPO_ROOT}/${doc}" "${RESOURCES_DIR}/"
done

# ── Info.plist ───────────────────────────────────────────────────────────────
# awk reads to the end: exiting early would kill otool with SIGPIPE, which
# pipefail turns into a silent exit.
MIN_MACOS="$(otool -l "${EXE}" | awk '/LC_BUILD_VERSION/ {b=1} b && !done && $1 == "minos" {print $2; done=1}')"
version_part() {
  sed -n "s/^set(VERSION_$1 \([0-9]*\))/\1/p" "${REPO_ROOT}/CMakeLists.txt"
}
VERSION="$(version_part MAJOR).$(version_part MINOR).$(version_part PATCH)"
[[ "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || VERSION="1.0"
cat > "${APP}/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key>
  <string>${APP_NAME}</string>
  <key>CFBundleDisplayName</key>
  <string>${APP_NAME}</string>
  <key>CFBundleIdentifier</key>
  <string>io.github.awest813.league-soccer</string>
  <key>CFBundleExecutable</key>
  <string>gameplayfootball</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleShortVersionString</key>
  <string>${VERSION}</string>
  <key>CFBundleVersion</key>
  <string>${VERSION}</string>
  <key>LSMinimumSystemVersion</key>
  <string>${MIN_MACOS:-11.0}</string>
  <key>LSApplicationCategoryType</key>
  <string>public.app-category.sports-games</string>
  <key>NSHighResolutionCapable</key>
  <true/>
</dict>
</plist>
EOF

# ── Libraries ────────────────────────────────────────────────────────────────
# Copy every non-system library the game loads (recursively) into Frameworks
# and point all references at @rpath/<name>.
is_system_library() {
  case "$1" in
    /System/*|/usr/lib/*) return 0 ;;
    *) return 1 ;;
  esac
}

# resolve_reference <reference> <path of the file that references it>
resolve_reference() {
  local ref="$1" from="$2" name candidate
  case "${ref}" in
    @rpath/*|@loader_path/*|@executable_path/*)
      name="${ref#*/}"
      for candidate in \
          "$(dirname "${from}")/${name}" \
          "${BUILD_DIR}/${name}" \
          "${HOMEBREW_PREFIX:-/opt/homebrew}/lib/${name}" \
          "/usr/local/lib/${name}"; do
        if [[ -f "${candidate}" ]]; then
          echo "${candidate}"
          return 0
        fi
      done
      return 1 ;;
    *) [[ -f "${ref}" ]] && echo "${ref}" ;;
  esac
}

dependencies() {  # the libraries a Mach-O file links, one per line
  otool -L "$1" | tail -n +2 | awk '{print $1}'
}

# Original location of each bundled library, so @rpath references inside it
# can be resolved against the Homebrew directory it came from.
BUNDLED_NAMES=()
BUNDLED_SOURCES=()
source_of() {
  local i
  for ((i = 0; i < ${#BUNDLED_NAMES[@]}; i++)); do
    if [[ "${BUNDLED_NAMES[$i]}" == "$1" ]]; then
      echo "${BUNDLED_SOURCES[$i]}"
      return 0
    fi
  done
  return 1
}

bundle_library() {  # bundle_library <name> <resolved path>
  cp -L "$2" "${FRAMEWORKS_DIR}/$1"
  chmod 644 "${FRAMEWORKS_DIR}/$1"
  install_name_tool -id "@rpath/$1" "${FRAMEWORKS_DIR}/$1"
  BUNDLED_NAMES+=("$1")
  BUNDLED_SOURCES+=("$2")
  QUEUE+=("${FRAMEWORKS_DIR}/$1")
  echo "    bundled $1"
}

process_queue() {
  local file origin ref name resolved
  while [[ ${#QUEUE[@]} -gt 0 ]]; do
    file="${QUEUE[0]}"
    QUEUE=("${QUEUE[@]:1}")
    origin="${file}"
    if [[ "${file}" != "${EXE}" ]]; then
      origin="$(source_of "$(basename "${file}")")"
    fi
    while IFS= read -r ref; do
      [[ -z "${ref}" ]] && continue
      is_system_library "${ref}" && continue
      name="$(basename "${ref}")"
      # A library's first entry is its own install name.
      [[ "${file}" != "${EXE}" && "${name}" == "$(basename "${file}")" ]] && continue
      if ! source_of "${name}" >/dev/null; then
        if ! resolved="$(resolve_reference "${ref}" "${origin}")"; then
          echo "Error: cannot find ${ref} (needed by ${file})" >&2
          exit 1
        fi
        bundle_library "${name}" "${resolved}"
      fi
      [[ "${ref}" == "@rpath/${name}" ]] ||
        install_name_tool -change "${ref}" "@rpath/${name}" "${file}"
    done < <(dependencies "${file}")
  done
}

QUEUE=("${EXE}")
process_queue

# Homebrew's sdl2 is sdl2-compat, which loads libSDL3 with dlopen when it
# starts, so otool does not list it. It looks beside itself first
# (@loader_path), so put libSDL3 in Frameworks too. Without it the app
# shows a "cannot load SDL3" dialog before the game starts.
if [[ -e "${FRAMEWORKS_DIR}/libSDL2-2.0.0.dylib" ]] &&
    grep -q "libSDL3" "${FRAMEWORKS_DIR}/libSDL2-2.0.0.dylib" &&
    ! source_of "libSDL3.0.dylib" >/dev/null; then
  sdl3=""
  for candidate in \
      "$(brew --prefix sdl3 2>/dev/null)/lib/libSDL3.0.dylib" \
      "${HOMEBREW_PREFIX:-/opt/homebrew}/lib/libSDL3.0.dylib" \
      "/usr/local/lib/libSDL3.0.dylib"; do
    if [[ -f "${candidate}" ]]; then
      sdl3="${candidate}"
      break
    fi
  done
  if [[ -z "${sdl3}" ]]; then
    echo "Error: libSDL2 is sdl2-compat, but libSDL3.0.dylib was not found (brew install sdl3)" >&2
    exit 1
  fi
  bundle_library "libSDL3.0.dylib" "${sdl3}"
  process_queue
  # sdl2-compat asks for the unversioned name; Homebrew provides it as a
  # symlink, and so does the bundle.
  ln -sf libSDL3.0.dylib "${FRAMEWORKS_DIR}/libSDL3.dylib"
  echo "    sdl2-compat looks for: $(strings "${FRAMEWORKS_DIR}/libSDL2-2.0.0.dylib" |
    grep -E 'SDL3\.(dylib|framework)|libSDL3' | sort -u | tr '\n' ' ')"
fi

# Bundled libraries find each other beside themselves; the game looks in
# ../Frameworks. Drop build-tree and Homebrew rpaths.
for lib in "${FRAMEWORKS_DIR}"/*.dylib; do
  [[ -e "${lib}" ]] || continue
  install_name_tool -add_rpath "@loader_path" "${lib}" 2>/dev/null || true
done
while IFS= read -r rpath; do
  case "${rpath}" in
    @executable_path/../Frameworks) ;;
    *) install_name_tool -delete_rpath "${rpath}" "${EXE}" 2>/dev/null || true ;;
  esac
done < <(otool -l "${EXE}" | awk '/cmd LC_RPATH/ {r=1} r && $1 == "path" {print $2; r=0}')
install_name_tool -add_rpath "@executable_path/../Frameworks" "${EXE}" 2>/dev/null || true

# ── Check and sign ───────────────────────────────────────────────────────────
leaks=0
for file in "${EXE}" "${FRAMEWORKS_DIR}"/*.dylib; do
  [[ -e "${file}" ]] || continue
  while IFS= read -r ref; do
    case "${ref}" in
      /System/*|/usr/lib/*|@rpath/*|@loader_path/*|@executable_path/*) ;;
      *) echo "Error: ${file#"${APP}/"} still links ${ref}" >&2; leaks=1 ;;
    esac
  done < <(dependencies "${file}")
done
[[ "${leaks}" -eq 0 ]] || exit 1

for lib in "${FRAMEWORKS_DIR}"/*.dylib; do
  [[ -f "${lib}" && ! -L "${lib}" ]] && codesign --force --sign - "${lib}"
done
codesign --force --sign - "${APP}"
codesign --verify --strict "${APP}"

if [[ "${ZIP}" == true ]]; then
  ZIP_PATH="${OUT_DIR}/GameplayFootball-macos-$(uname -m).zip"
  rm -f "${ZIP_PATH}"
  ditto -c -k --sequesterRsrc --keepParent "${APP}" "${ZIP_PATH}"
  echo "==> Wrote ${ZIP_PATH}"
fi

echo "==> macOS packaging complete: ${APP}"
