#!/usr/bin/env bash
set -euo pipefail

root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
src=${PPSSPP_SOURCE_DIR:-${1:-}}

[[ -n $src && -d $src/.git ]] || {
  echo "usage: PPSSPP_SOURCE_DIR=/path/to/ppsspp $0" >&2
  exit 2
}

python3 "$root/tools/ps5/apply_ppsspp_native.py" "$src"
echo "PPSSPP v1.20.4 native PS5 engine transformation complete"
