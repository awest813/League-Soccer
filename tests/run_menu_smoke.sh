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

runner=()
if command -v stdbuf >/dev/null 2>&1 && command -v timeout >/dev/null 2>&1; then
  runner=(stdbuf -oL -eL timeout "${timeout_seconds}s")
elif command -v gstdbuf >/dev/null 2>&1 && command -v gtimeout >/dev/null 2>&1; then
  runner=(gstdbuf -oL -eL gtimeout "${timeout_seconds}s")
fi

if command -v xvfb-run >/dev/null 2>&1; then
  if [[ ${#runner[@]} -gt 0 ]]; then
    runner=(xvfb-run -a "${runner[@]}")
  else
    runner=(xvfb-run -a)
  fi
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
