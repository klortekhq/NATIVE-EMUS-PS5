#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/dolphin-vulkan}"
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
  -DENABLE_VULKAN=ON \
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

"$CMAKE" --build "$BUILD" --target videovulkan --parallel "${JOBS:-2}"

ARCHIVE="$(find "$BUILD" -type f \( -name 'libvideovulkan.a' -o -name 'videovulkan.a' \) -print -quit)"
[[ -n "$ARCHIVE" && -s "$ARCHIVE" ]] || {
  echo "Dolphin PS5 Vulkan archive missing" >&2
  exit 3
}

CONTENTS="$("$AR" t "$ARCHIVE")"
grep -q 'VulkanLoader.cpp' <<<"$CONTENTS" || {
  echo "Dolphin Vulkan archive lacks Vulkan loader" >&2
  exit 4
}
grep -q 'VKSwapChain.cpp' <<<"$CONTENTS" || {
  echo "Dolphin Vulkan archive lacks swap-chain implementation" >&2
  exit 5
}
grep -q 'VulkanContext.cpp' <<<"$CONTENTS" || {
  echo "Dolphin Vulkan archive lacks Vulkan context implementation" >&2
  exit 6
}

cp "$ARCHIVE" "$OUT/artifacts/libdolphin_vulkan_ps5.a"
sha256sum "$OUT/artifacts/libdolphin_vulkan_ps5.a" > "$OUT/artifacts/SHA256SUMS"

echo "Dolphin PS5 Vulkan backend compile gate complete"
echo "pin=$PIN"
echo "NOTE: this is compile evidence only; RADV static linkage and VK_KHR_display runtime presentation remain separate gates."
