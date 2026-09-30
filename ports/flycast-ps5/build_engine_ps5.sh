#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/flycast}"
SRC="$OUT/src/flycast"
PIN="e36e9df2dcc1487acdb1dc7725766f1f5ba029b5"
REPO="https://github.com/flyinghead/flycast.git"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public ps5-payload SDK root}"
CMAKE="${PS5_CMAKE:-$PS5_PAYLOAD_SDK/bin/prospero-cmake}"
AR="${PS5_AR:-$PS5_PAYLOAD_SDK/bin/prospero-ar}"

[[ -x "$CMAKE" ]] || { echo "missing prospero-cmake: $CMAKE" >&2; exit 2; }
[[ -x "$AR" ]] || { echo "missing prospero-ar: $AR" >&2; exit 2; }

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/build" "$OUT/artifacts"

git clone --filter=blob:none "$REPO" "$SRC"
git -C "$SRC" checkout --detach "$PIN"
git -C "$SRC" submodule update --init --recursive --depth 1

python3 "$ROOT/tools/ps5/apply_flycast_native.py" "$SRC"

export NATIVE_EMUS_ROOT="$ROOT"

"$CMAKE" -S "$SRC" -B "$OUT/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DFLYCAST_PS5=ON \
  -DLIBRETRO=OFF \
  -DUSE_VULKAN=OFF \
  -DUSE_OPENGL=OFF \
  -DUSE_DX9=OFF \
  -DUSE_DX11=OFF \
  -DUSE_OPENMP=OFF \
  -DUSE_BREAKPAD=OFF \
  -DUSE_LUA=OFF \
  -DUSE_DISCORD=OFF \
  -DUSE_ALSA=OFF \
  -DUSE_LIBAO=OFF \
  -DUSE_OSS=OFF \
  -DUSE_PULSEAUDIO=OFF \
  -DUSE_HOST_LIBZIP=OFF \
  -DUSE_HOST_LIBCHDR=OFF \
  -DUSE_LIBCDIO=OFF \
  -DENABLE_CTEST=OFF \
  -DENABLE_GDB_SERVER=OFF \
  -DHAVE_MEMCPY_S=OFF

"$CMAKE" --build "$OUT/build" --target flycast --parallel "${JOBS:-2}"

ENGINE="$(find "$OUT/build" -type f -name 'libflycast_ps5_engine.a' -print -quit)"
[[ -n "$ENGINE" && -s "$ENGINE" ]] || {
  echo "Flycast PS5 engine archive missing" >&2
  exit 3
}

CONTENTS="$("$AR" t "$ENGINE")"
grep -q 'rec_x64.cpp.o' <<<"$CONTENTS" || {
  echo "Flycast archive does not contain the x86-64 SH4 recompiler" >&2
  exit 4
}
grep -q 'ps5_vmem.cpp.o' <<<"$CONTENTS" || {
  echo "Flycast archive does not contain the PS5 vmem/JIT adapter" >&2
  exit 5
}
grep -q 'driver.cpp.o' <<<"$CONTENTS" || {
  echo "Flycast archive does not contain the SH4 dynarec driver" >&2
  exit 6
}

cp "$ENGINE" "$OUT/artifacts/libflycast_ps5_engine.a"
sha256sum "$OUT/artifacts/libflycast_ps5_engine.a" > "$OUT/artifacts/SHA256SUMS"

echo "Flycast PS5 rec-x64 engine build complete"
echo "pin=$PIN"
echo "archive=$OUT/artifacts/libflycast_ps5_engine.a"
