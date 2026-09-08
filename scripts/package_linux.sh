#!/usr/bin/env bash
# scripts/package_linux.sh – package League-Soccer / Gameplay Football for Linux.
#
# Assembles a standalone, portable Linux release directory containing the
# executable, launcher wrapper script with LD_LIBRARY_PATH and working directory setup,
# runtime configuration, and game assets.
#
# Usage:
#   scripts/package_linux.sh --build-dir <path> --out-dir <path> [--strip]
#
# Options:
#   --build-dir <path>   Directory containing the built gameplayfootball executable
#   --out-dir <path>     Target output directory for the packaged release
#   --strip              Strip debug symbols from the executable (if strip is available)
#   --help, -h           Show this help message

set -euo pipefail

BUILD_DIR=""
OUT_DIR=""
STRIP=false

usage() {
  sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'
  exit 0
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir) BUILD_DIR="$2"; shift 2 ;;
    --out-dir)   OUT_DIR="$2"; shift 2 ;;
    --strip)     STRIP=true; shift ;;
    --help|-h)   usage ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

if [[ -z "${BUILD_DIR}" || -z "${OUT_DIR}" ]]; then
  echo "Error: --build-dir and --out-dir are required." >&2
  usage
fi

if [[ ! -f "${BUILD_DIR}/gameplayfootball" ]]; then
  echo "Error: ${BUILD_DIR}/gameplayfootball not found." >&2
  exit 1
fi

echo "==> Packaging Linux build from ${BUILD_DIR} into ${OUT_DIR}"

# Find repository root (script directory parent)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Recreate target output directory
rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

# Copy the executable
cp "${BUILD_DIR}/gameplayfootball" "${OUT_DIR}/"
chmod +x "${OUT_DIR}/gameplayfootball"

# Optionally strip debug symbols
if [[ "${STRIP}" == true ]]; then
  if command -v strip >/dev/null 2>&1; then
    echo "==> Stripping executable symbols…"
    strip "${OUT_DIR}/gameplayfootball" || true
  else
    echo "==> Warning: strip requested but 'strip' command not found. Skipping."
  fi
fi

# Create launcher wrapper script
cat << 'EOF' > "${OUT_DIR}/gameplayfootball.sh"
#!/usr/bin/env bash
# Portable launch wrapper for Gameplay Football / League-Soccer on Linux
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="${HERE}:${HERE}/lib:${LD_LIBRARY_PATH:-}"
cd "${HERE}"
exec "${HERE}/gameplayfootball" "$@"
EOF
chmod +x "${OUT_DIR}/gameplayfootball.sh"

# Copy configurations and assets
if [[ -f "${BUILD_DIR}/football.config" ]]; then
  cp "${BUILD_DIR}/football.config" "${OUT_DIR}/"
elif [[ -f "${REPO_ROOT}/data/football.config" ]]; then
  cp "${REPO_ROOT}/data/football.config" "${OUT_DIR}/"
fi

for d in media databases locale; do
  if [[ -d "${BUILD_DIR}/${d}" ]]; then
    cp -R "${BUILD_DIR}/${d}" "${OUT_DIR}/"
  elif [[ -d "${REPO_ROOT}/data/${d}" ]]; then
    cp -R "${REPO_ROOT}/data/${d}" "${OUT_DIR}/"
  fi
done

# Copy nested data dir expected by legacy path lookups
mkdir -p "${OUT_DIR}/data"
if [[ -f "${BUILD_DIR}/data/football.config" ]]; then
  cp "${BUILD_DIR}/data/football.config" "${OUT_DIR}/data/"
elif [[ -f "${REPO_ROOT}/data/football.config" ]]; then
  cp "${REPO_ROOT}/data/football.config" "${OUT_DIR}/data/"
fi

if [[ -d "${BUILD_DIR}/data/locale" ]]; then
  cp -R "${BUILD_DIR}/data/locale" "${OUT_DIR}/data/"
elif [[ -d "${REPO_ROOT}/data/locale" ]]; then
  cp -R "${REPO_ROOT}/data/locale" "${OUT_DIR}/data/"
fi

# Copy any shared libraries if present in build directory
mkdir -p "${OUT_DIR}/lib"
find "${BUILD_DIR}" -maxdepth 2 -type f \( -name "*.so" -o -name "*.so.*" \) -exec cp -d {} "${OUT_DIR}/lib/" \; 2>/dev/null || true
# Clean up empty lib directory if no .so files were copied
rmdir "${OUT_DIR}/lib" 2>/dev/null || true

# Copy documentation and license if available
if [[ -f "${REPO_ROOT}/LICENSE" ]]; then
  cp "${REPO_ROOT}/LICENSE" "${OUT_DIR}/"
fi
if [[ -f "${REPO_ROOT}/README.md" ]]; then
  cp "${REPO_ROOT}/README.md" "${OUT_DIR}/"
fi

ITEM_COUNT="$(find "${OUT_DIR}" -mindepth 1 | wc -l)"
echo "==> Linux packaging complete: ${OUT_DIR} (${ITEM_COUNT} items staged)"
