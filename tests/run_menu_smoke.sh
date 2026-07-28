#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 4 ]]; then
  echo "usage: $0 <executable> <config> <timeout-seconds> <expected-marker> [marker...]" >&2
  exit 2
fi

executable="$1"
config="$2"
timeout_seconds="$3"
shift 3

log_file="$(mktemp)"
cleanup() {
  rm -f "$log_file"
}
trap cleanup EXIT

runner=(stdbuf -oL -eL timeout "${timeout_seconds}s")
if command -v xvfb-run >/dev/null 2>&1; then
  runner=(xvfb-run -a "${runner[@]}")
fi

if ! "${runner[@]}" "$executable" "$config" >"$log_file" 2>&1; then
  echo "menu smoke run failed for config: $config" >&2
  cat "$log_file" >&2
  exit 1
fi

for marker in "$@"; do
  if ! grep -Fq "$marker" "$log_file"; then
    echo "missing smoke marker: $marker" >&2
    cat "$log_file" >&2
    exit 1
  fi
done

cat "$log_file"
