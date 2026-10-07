#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/dolphin}"
SRC="$OUT/src/dolphin"
BUILD="$OUT/build"
PIN="c6630001e05780b7c03e661a4a539b59ef716ebc"
REPO="https://github.com/libretro/dolphin.git"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public ps5-payload SDK root}"
CMAKE="${PS5_CMAKE:-$PS5_PAYLOAD_SDK/bin/prospero-cmake}"
AR="${PS5_AR:-$PS5_PAYLOAD_SDK/bin/prospero-ar}"

[[ -x "$CMAKE" ]] || { echo "missing prospero-cmake" >&2; exit 2; }
[[ -x "$AR" ]] || { echo "missing prospero-ar" >&2; exit 2; }

rm -rf "$OUT"
mkdir -p "$OUT/src" "$BUILD" "$OUT/artifacts"

git clone --filter=blob:none "$REPO" "$SRC"
git -C "$SRC" checkout --detach "$PIN"
git -C "$SRC" submodule update --init --recursive --depth 1

python3 "$ROOT/tools/ps5/apply_dolphin_native.py" "$SRC"

"$CMAKE" -S "$SRC" -B "$BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
  -DCMAKE_C_FLAGS_RELEASE="-O3 -DNDEBUG -march=znver2 -msse4.1 -mavx2" \
  -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG -march=znver2 -msse4.1 -mavx2 -mno-vzeroupper -I$ROOT/runtime/include" \
  -DDOLPHIN_PS5=ON \
  -DLIBRETRO=ON \
  -DENABLE_GENERIC=OFF \
  -DENABLE_HEADLESS=ON \
  -DENABLE_QT=OFF \
  -DENABLE_NOGUI=OFF \
  -DENABLE_TESTS=OFF \
  -DENABLE_VULKAN=OFF \
  -DENABLE_EGL=OFF \
  -DENABLE_X11=OFF \
  -DENABLE_SDL=OFF \
  -DENABLE_CUBEB=OFF \
  -DENABLE_ALSA=OFF \
  -DENABLE_PULSEAUDIO=OFF \
  -DENABLE_LLVM=OFF \
  -DENABLE_AUTOUPDATE=OFF \
  -DENABLE_ANALYTICS=OFF \
  -DENCODE_FRAMEDUMPS=OFF \
  -DUSE_DISCORD_PRESENCE=OFF \
  -DUSE_RETRO_ACHIEVEMENTS=OFF \
  -DUSE_MGBA=OFF \
  -DUSE_UPNP=OFF \
  -DUSE_SYSTEM_LIBS=OFF

"$CMAKE" --build "$BUILD" --target core --parallel "${JOBS:-2}"

ENGINE="$(find "$BUILD" -type f \( -name 'libcore.a' -o -name 'core.a' \) -print -quit)"
[[ -n "$ENGINE" && -s "$ENGINE" ]] || {
  echo "Dolphin PS5 core archive missing" >&2
  exit 3
}

CONTENTS="$("$AR" t "$ENGINE")"
grep -q 'Jit.cpp' <<<"$CONTENTS" || {
  echo "Dolphin archive does not contain PowerPC Jit64" >&2
  exit 4
}
grep -q 'JitAsm.cpp' <<<"$CONTENTS" || {
  echo "Dolphin archive does not contain Jit64 assembler" >&2
  exit 5
}
grep -q 'DSPEmitter.cpp' <<<"$CONTENTS" || {
  echo "Dolphin archive does not contain x64 DSP JIT" >&2
  exit 6
}

cp "$ENGINE" "$OUT/artifacts/libdolphin_jit64_ps5.a"
sha256sum "$OUT/artifacts/libdolphin_jit64_ps5.a" > "$OUT/artifacts/SHA256SUMS"

echo "Dolphin PS5 Jit64 + DSP x64 engine build complete"
echo "pin=$PIN"
