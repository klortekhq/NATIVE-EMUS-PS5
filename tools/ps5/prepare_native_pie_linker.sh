#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${1:-$ROOT/build/ps5/native-linker}"
PIN="dd44bbdc75437332ed22e3ba95126733419ef25a"
SHA256="fbc52518761b58b703a2225c847c5f44a81523cd0e751755adf890ae0f0dd74c"
URL="https://raw.githubusercontent.com/BlackBearReloaded/ps5-native-app-boilerplate/$PIN/tooling/native/ps5-pie.ld"
SCRIPT="$OUT/ps5-pie.ld"

mkdir -p "$OUT"

if [[ ! -s "$SCRIPT" ]] || ! echo "$SHA256  $SCRIPT" | sha256sum -c - >/dev/null 2>&1; then
  tmp="$SCRIPT.tmp"
  rm -f "$tmp"
  curl --fail --location --retry 3 --retry-all-errors "$URL" -o "$tmp"
  echo "$SHA256  $tmp" | sha256sum -c -
  mv "$tmp" "$SCRIPT"
fi

echo "$SHA256  $SCRIPT" | sha256sum -c - >&2
printf '%s\n' "$SCRIPT"
