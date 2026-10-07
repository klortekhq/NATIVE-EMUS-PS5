#!/usr/bin/env bash
set -euo pipefail

root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
src=${DOLPHIN_SOURCE_DIR:-${1:-}}

: "${PS5_PAYLOAD_SDK:?PS5_PAYLOAD_SDK must point at the PS5 SDK}"
[[ -n $src && -d $src/.git ]] || {
  echo "set DOLPHIN_SOURCE_DIR to the pinned Dolphin checkout" >&2
  exit 2
}

export NATIVE_EMUS_ROOT="$root"

python3 "$root/tools/ps5/apply_dolphin_native.py" "$src"

build_dir="$root/build/ps5/dolphin-engine"
rm -rf "$build_dir"

cmake -S "$src" -B "$build_dir" -G Ninja   -DCMAKE_TOOLCHAIN_FILE="$root/tooling/dolphin/ps5-engine-toolchain.cmake"   -DCMAKE_BUILD_TYPE=Release   -DENABLE_QT=OFF   -DENABLE_SDL=OFF   -DENABLE_X11=OFF   -DENABLE_EGL=OFF   -DENABLE_TESTS=OFF   -DENABLE_CLI_TOOL=OFF   -DENABLE_AUTOUPDATE=OFF   -DENABLE_ANALYTICS=OFF   -DUSE_DISCORD_PRESENCE=OFF   -DUSE_RETRO_ACHIEVEMENTS=OFF   -DUSE_MGBA=OFF   -DENABLE_LLVM=OFF   -DENCODE_FRAMEDUMPS=OFF   -DUSE_UPNP=OFF   -DUSE_SYSTEM_LIBS=OFF

# Compile the core libraries first. This is deliberately not a libretro target.
cmake --build "$build_dir" --parallel "${JOBS:-16}" --target Core VideoCommon Common

echo "Dolphin native engine libraries built; standalone PS5 title adapter is the next gate."
