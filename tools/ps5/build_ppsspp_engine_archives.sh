#!/usr/bin/env bash
set -euo pipefail

root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
src=${PPSSPP_SOURCE_DIR:-${1:-}}

: "${PS5_PAYLOAD_SDK:?PS5_PAYLOAD_SDK must point at the PS5 SDK}"
[[ -n $src && -d $src ]] || {
  echo "set PPSSPP_SOURCE_DIR to a prepared PPSSPP v1.20.4 tree" >&2
  exit 2
}
[[ -f $src/.native-emus-ps5-engine-patch ]] || {
  echo "run tools/ps5/prepare_ppsspp_native.sh first" >&2
  exit 2
}

export NATIVE_EMUS_ROOT="$root"

build="$root/build/ps5/ppsspp-engine"
rm -rf "$build"

cmake -S "$src" -B "$build" -G Ninja   -DCMAKE_TOOLCHAIN_FILE="$root/tooling/ppsspp/ps5-engine-toolchain.cmake"   -DCMAKE_BUILD_TYPE=Release   -DLIBRETRO=OFF   -DHEADLESS=ON   -DUNITTEST=OFF   -DUSE_FFMPEG=OFF   -DUSE_DISCORD=OFF   -DUSE_MINIUPNPC=OFF   -DUSING_GLES2=OFF   -DUSING_X11_VULKAN=OFF   -DUSE_WAYLAND_WSI=OFF   -DUSE_SYSTEM_LIBPNG=OFF   -DUSE_SYSTEM_ZSTD=OFF   -DUSE_SYSTEM_LIBZIP=OFF   -DUSE_SYSTEM_FREETYPE=OFF

# Compile-only gate. Common/Core contain PPSSPP's MIPS x86-64 JIT and PSP HLE.
cmake --build "$build" --target Common Core --parallel "${JOBS:-16}"

find "$build" -type f \( -name 'libCommon.a' -o -name 'libCore.a' \) -print
