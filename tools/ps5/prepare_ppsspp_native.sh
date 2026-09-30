#!/usr/bin/env bash
set -euo pipefail

root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
src=${PPSSPP_SOURCE_DIR:-${1:-}}
revision=fa50bb1976065c4f8b1b47af227d367fe9771555

[[ -n $src && -d $src/.git ]] || {
  echo "usage: PPSSPP_SOURCE_DIR=/path/to/ppsspp $0" >&2
  exit 2
}

got=$(git -C "$src" rev-parse HEAD)
[[ $got == "$revision" ]] || {
  echo "PPSSPP source is $got, expected $revision (v1.20.4)" >&2
  exit 2
}

git -C "$src" diff --quiet || {
  echo "PPSSPP source has local changes; use a clean pinned tree" >&2
  exit 2
}

patch="$root/patches/ppsspp/v1.20.4-ps5-engine.patch"
git -C "$src" apply --check "$patch"
git -C "$src" apply "$patch"
printf '%s\n' "$revision" > "$src/.native-emus-ps5-engine-patch"

echo "PPSSPP v1.20.4 native PS5 engine patch applied"
