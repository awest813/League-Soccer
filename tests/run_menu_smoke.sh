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
xvfb_pid=""
cleanup() {
  rm -f "$log_file"
  if [[ -n "$xvfb_pid" ]]; then
    kill "$xvfb_pid" 2>/dev/null || true
    wait "$xvfb_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT

runner=()
if command -v stdbuf >/dev/null 2>&1 && command -v timeout >/dev/null 2>&1; then
  runner=(stdbuf -oL -eL timeout --kill-after=10s "${timeout_seconds}s")
elif command -v gstdbuf >/dev/null 2>&1 && command -v gtimeout >/dev/null 2>&1; then
  runner=(gstdbuf -oL -eL gtimeout -k 10s "${timeout_seconds}s")
fi

if command -v xvfb-run >/dev/null 2>&1; then
  if [[ ${#runner[@]} -gt 0 ]]; then
    runner=(xvfb-run -a "${runner[@]}")
  else
    runner=(xvfb-run -a)
  fi
elif [[ -z "${DISPLAY:-}" ]] && command -v Xvfb >/dev/null 2>&1; then
  # Void Linux and Alpine ship Xvfb without Debian's xvfb-run wrapper.
  # -displayfd makes Xvfb pick a free display and print it once it is ready.
  display_file="$(mktemp)"
  Xvfb -displayfd 3 -screen 0 1280x1024x24 -nolisten tcp 3>"$display_file" >/dev/null 2>&1 &
  xvfb_pid=$!
  for _ in $(seq 100); do
    [[ -s "$display_file" ]] && break
    sleep 0.1
  done
  if [[ -s "$display_file" ]]; then
    export DISPLAY=":$(head -n1 "$display_file")"
  else
    echo "Xvfb did not start; running without a virtual display" >&2
  fi
  rm -f "$display_file"
fi

if ! "${runner[@]+"${runner[@]}"}" "$executable" "$config" >"$log_file" 2>&1; then
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
