#!/usr/bin/env bash
# run.sh – launch the built game, making sure its runtime assets are found.
#
# The game reads media/, databases/, locale/ and football.config relative to
# its working directory. CMake copies them next to the binary, so we launch
# from build/ (or build/Debug on single-config generators that nest it).
#
# Usage:
#   ./run.sh [options] [-- extra args passed to the game]
#
# Options:
#   --debug      Run the Debug binary (build/gameplayfootball in either layout)
#   --help, -h   Show this message

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
  sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'
  exit 0
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --debug|-d) shift ;;
    --help|-h)  usage ;;
    --)         shift; break ;;
    *)          break ;;
  esac
done

# Remaining args are forwarded to the game.
EXTRA_ARGS=("$@")

# Locate the executable + assets. The CMake POST_BUILD step copies assets
# beside the binary, so wherever gameplayfootball is, the assets are too.
BIN=""
for candidate in \
    "${SCRIPT_DIR}/build/gameplayfootball" \
    "${SCRIPT_DIR}/build/Debug/gameplayfootball"; do
  if [[ -x "${candidate}" ]]; then
    BIN="${candidate}"
    break
  fi
done

if [[ -z "${BIN}" ]]; then
  echo "Game executable not found under build/." >&2
  echo "Build it first with:  ./build.sh" >&2
  exit 1
fi

RUN_DIR="$(dirname "${BIN}")"

echo "Launching ${BIN} from ${RUN_DIR}"
cd "${RUN_DIR}"
exec ./gameplayfootball "${EXTRA_ARGS[@]}"
