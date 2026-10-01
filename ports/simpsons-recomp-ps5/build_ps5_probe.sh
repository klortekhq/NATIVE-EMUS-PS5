#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PIN="15feabc95291a7a50851a9a184c5bd78d2dd1f4d"
SDK="${PS5_PAYLOAD_SDK:-/opt/ps5-payload-sdk}"
SRC="${SIMPSONS_RECOMP_SRC:-$ROOT/build/ps5/simpsons-recomp/src}"
BUILD="${SIMPSONS_RECOMP_BUILD:-$ROOT/build/ps5/simpsons-recomp/probe}"

test -x "$SDK/bin/prospero-clang++" || {
  echo "PS5_PAYLOAD_SDK does not contain prospero-clang++: $SDK" >&2
  exit 2
}

if [[ ! -d "$SRC/.git" ]]; then
  mkdir -p "$(dirname "$SRC")"
  git clone --filter=blob:none https://github.com/YesterMester/TheSimpsonsGameRecomp.git "$SRC"
  git -C "$SRC" checkout --detach "$PIN"
fi

actual="$(git -C "$SRC" rev-parse HEAD)"
[[ "$actual" == "$PIN" ]] || {
  echo "wrong Simpsons upstream: $actual (expected $PIN)" >&2
  exit 3
}

marker="$SRC/.native-emus-ps5-simpsons"
if [[ ! -f "$marker" ]]; then
  python3 "$ROOT/tools/ps5/apply_simpsons_recomp_ps5.py" "$SRC"
else
  [[ "$(tr -d '\r\n' < "$marker")" == "$PIN" ]] || {
    echo "stale PS5 transform marker" >&2
    exit 4
  }
fi

export PS5_PAYLOAD_SDK="$SDK"
export NATIVE_EMUS_ROOT="$ROOT"

# The public SDK wrapper injects crt1.o even for preprocessing-only
# invocations. Real target compilation below is the authoritative platform gate.

cmake -S "$SRC/simpsons" -B "$BUILD" -G Ninja   -DCMAKE_TOOLCHAIN_FILE="$ROOT/tooling/simpsons/ps5-toolchain.cmake"   -DREXSDK_DIR="$SRC/tools/rexglue-sdk"   -DCMAKE_BUILD_TYPE=Release   -DREXGLUE_BUILD_TESTS=OFF   -DREXGLUE_ENABLE_TRACY=OFF   -DREXGLUE_ENABLE_FIDELITYFX=OFF   -DREXGLUE_ENABLE_PERF_COUNTERS=OFF

# First cross-build gate: compile the PS5-facing ReXGlue stack without linking
# the final game. This deliberately covers the platform core, fullscreen Vulkan
# presenter, XMA/audio bridge and DualSense bridge before the 82k guest TUs.
cmake --build "$BUILD" --target rexcore rexui rexinput rexaudio -- -j2

echo "SIMpsons PS5 platform compile probe: PASS"
