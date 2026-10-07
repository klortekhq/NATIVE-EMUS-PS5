#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/mgba}"
CORE="$OUT/src/mgba"
BUILD="$OUT/core-build"
ARTIFACTS="$OUT/artifacts"

PIN="c3c8e5e813f245028de118a56734e1dc0f35ce2a"
REPO="https://github.com/mgba-emu/mgba.git"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public ps5-payload SDK root}"

PS5_CMAKE="${PS5_CMAKE:-$PS5_PAYLOAD_SDK/bin/prospero-cmake}"
for tool in "$PS5_CMAKE" git; do
  if [[ "$tool" == "git" ]]; then
    command -v git >/dev/null 2>&1 || { echo "missing tool: git" >&2; exit 2; }
  else
    [[ -x "$tool" ]] || { echo "missing tool: $tool" >&2; exit 2; }
  fi
done

rm -rf "$OUT"
mkdir -p "$OUT/src" "$ARTIFACTS"

git init -q "$CORE"
git -C "$CORE" remote add origin "$REPO"
git -C "$CORE" fetch --quiet --depth=1 origin "$PIN"
git -C "$CORE" -c advice.detachedHead=false checkout --quiet --detach FETCH_HEAD
[[ "$(git -C "$CORE" rev-parse HEAD)" == "$PIN" ]] || {
  echo "mGBA pin mismatch" >&2
  exit 3
}

"$PS5_CMAKE" -S "$CORE" -B "$BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_LIBRETRO=OFF \
  -DSKIP_LIBRARY=OFF \
  -DM_CORE_GBA=ON \
  -DM_CORE_GB=OFF \
  -DBUILD_QT=OFF \
  -DBUILD_SDL=OFF \
  -DBUILD_HEADLESS=OFF \
  -DBUILD_EXAMPLE=OFF \
  -DBUILD_PYTHON=OFF \
  -DBUILD_PERF=OFF \
  -DBUILD_TEST=OFF \
  -DBUILD_SUITE=OFF \
  -DBUILD_CINEMA=OFF \
  -DBUILD_STATIC=ON \
  -DBUILD_SHARED=OFF \
  -DBUILD_GL=OFF \
  -DBUILD_GLES2=OFF \
  -DBUILD_GLES3=OFF \
  -DDISABLE_DEPS=ON \
  -DENABLE_DEBUGGERS=OFF \
  -DENABLE_GDB_STUB=OFF \
  -DENABLE_SCRIPTING=OFF \
  -DUSE_FFMPEG=OFF \
  -DUSE_ZLIB=OFF \
  -DUSE_MINIZIP=OFF \
  -DUSE_PNG=OFF \
  -DUSE_LIBZIP=OFF \
  -DUSE_SQLITE3=OFF \
  -DUSE_ELF=OFF \
  -DUSE_LUA=OFF \
  -DUSE_JSON_C=OFF \
  -DUSE_FREETYPE=OFF \
  -DUSE_LZMA=OFF \
  -DUSE_DISCORD_RPC=OFF \
  -DBUILD_LTO=OFF \
  -DCMAKE_C_FLAGS="-O3 -DNDEBUG -march=znver2 -msse4.1 -mavx2 -mno-vzeroupper"

"$PS5_CMAKE" --build "$BUILD" --target mgba --parallel "${JOBS:-2}"

CORE_LIB="$(find "$BUILD" -type f -name 'libmgba.a' -print -quit)"
[[ -n "$CORE_LIB" && -s "$CORE_LIB" ]] || {
  echo "mGBA native static archive missing" >&2
  exit 4
}

cp "$CORE_LIB" "$ARTIFACTS/libmgba_gba_ps5.a"

if command -v nm >/dev/null 2>&1; then
  for sym in mCoreFindVF mCoreConfigInit mCoreInitConfig; do
    nm "$ARTIFACTS/libmgba_gba_ps5.a" 2>/dev/null | grep -q "[[:space:]]$sym$" || {
      echo "required mGBA core symbol missing: $sym" >&2
      exit 5
    }
  done
fi

python3 "$ROOT/tools/ps5/write_artifact_manifest.py" \
  --output-dir "$ARTIFACTS" \
  --system "Nintendo Game Boy Advance" \
  --core "mGBA" \
  --upstream "mgba-emu/mgba" \
  --pin "$PIN" \
  --cpu-backend "upstream ARM7TDMI interpreter (direct mGBA core library)" \
  --graphics-backend "software framebuffer; native PS5 frontend not linked yet" \
  --artifact "$ARTIFACTS/libmgba_gba_ps5.a"

sha256sum "$ARTIFACTS/libmgba_gba_ps5.a" "$ARTIFACTS/BUILD-MANIFEST.json" > "$ARTIFACTS/SHA256SUMS"

echo "mGBA GBA core -> PS5 static archive: PASS"
