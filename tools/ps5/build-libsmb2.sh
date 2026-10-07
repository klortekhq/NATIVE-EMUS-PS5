#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/libsmb2}"
PIN="21f3c8f694d62222ee2c5f86327778a87728cf6e"
REPO="https://github.com/sahlberg/libsmb2.git"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public PS5 Payload SDK root}"

CMAKE="${PS5_CMAKE:-$PS5_PAYLOAD_SDK/bin/prospero-cmake}"
[[ -x "$CMAKE" ]] || {
  echo "missing prospero-cmake: $CMAKE" >&2
  exit 2
}

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/build" "$OUT/install"

git clone --filter=blob:none "$REPO" "$OUT/src/libsmb2"
git -C "$OUT/src/libsmb2" checkout --detach "$PIN"

"$CMAKE" -S "$OUT/src/libsmb2" -B "$OUT/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$OUT/install" \
  -DBUILD_SHARED_LIBS=OFF \
  -DENABLE_EXAMPLES=OFF \
  -DENABLE_LIBDCERPC=OFF \
  -DENABLE_LIBKRB5=OFF \
  -DENABLE_GSSAPI=OFF

"$CMAKE" --build "$OUT/build" --parallel "${JOBS:-2}"
"$CMAKE" --install "$OUT/build"

LIB="$(find "$OUT/install" -type f -name 'libsmb2.a' -print -quit)"
[[ -n "$LIB" && -s "$LIB" ]] || {
  echo "libsmb2 static archive was not installed" >&2
  exit 3
}

test -f "$OUT/install/include/smb2/libsmb2.h"
test -f "$OUT/install/include/smb2/smb2.h"

cp "$LIB" "$OUT/libsmb2.ps5.a"
sha256sum "$OUT/libsmb2.ps5.a" > "$OUT/SHA256SUMS"

echo "libsmb2 PS5 build complete"
echo "pin=$PIN"
echo "archive=$OUT/libsmb2.ps5.a"
