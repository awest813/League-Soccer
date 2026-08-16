#!/usr/bin/env bash
# scripts/package_macos.sh – package League-Soccer for macOS
#
# Assembles a standalone macOS distribution containing the executable,
# runtime assets, and copied Homebrew dynamic libraries if available.
#
# Usage:
#   scripts/package_macos.sh --build-dir <path> --out-dir <path>

set -euo pipefail

BUILD_DIR=""
OUT_DIR=""

usage() {
  echo "Usage: $0 --build-dir <path> --out-dir <path>"
  exit 0
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir) BUILD_DIR="$2"; shift 2 ;;
    --out-dir)   OUT_DIR="$2"; shift 2 ;;
    --help|-h)   usage ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

if [[ -z "${BUILD_DIR}" || -z "${OUT_DIR}" ]]; then
  echo "Error: --build-dir and --out-dir are required." >&2
  usage
fi

if [[ ! -x "${BUILD_DIR}/gameplayfootball" ]]; then
  echo "Error: ${BUILD_DIR}/gameplayfootball not found or not executable." >&2
  exit 1
fi

echo "==> Packaging macOS build from ${BUILD_DIR} into ${OUT_DIR}"

mkdir -p "${OUT_DIR}"

# Copy the executable
cp "${BUILD_DIR}/gameplayfootball" "${OUT_DIR}/"

# Copy the assets and configurations
cp "${BUILD_DIR}/football.config" "${OUT_DIR}/" || true

# Copy directories
for d in media databases locale; do
  if [[ -d "${BUILD_DIR}/${d}" ]]; then
    cp -R "${BUILD_DIR}/${d}" "${OUT_DIR}/"
  fi
done

# Copy nested data dir if exists
mkdir -p "${OUT_DIR}/data"
if [[ -f "${BUILD_DIR}/data/football.config" ]]; then
  cp "${BUILD_DIR}/data/football.config" "${OUT_DIR}/data/"
fi
if [[ -d "${BUILD_DIR}/data/locale" ]]; then
  cp -R "${BUILD_DIR}/data/locale" "${OUT_DIR}/data/"
fi

echo "macOS packaging complete: ${OUT_DIR}"
