#!/usr/bin/env bash
set -euo pipefail

# Native PS5 bring-up build for YesterMester/TheSimpsonsGameRecomp.
# Usage:
#   NATIVE_EMUS_ROOT=/path/to/NATIVE-EMUS-PS5 \
#   PS5_PAYLOAD_SDK=/path/to/ps5-payload-sdk \
#   ./build_ps5.sh /path/to/TheSimpsonsGameRecomp

SRC="${1:?usage: build_ps5.sh /path/to/TheSimpsonsGameRecomp}"
ROOT="${NATIVE_EMUS_ROOT:?NATIVE_EMUS_ROOT is required}"
SDK="${PS5_PAYLOAD_SDK:?PS5_PAYLOAD_SDK is required}"

PORT="${ROOT}/ports/simpsons-recomp-ps5"
TOOLCHAIN="${ROOT}/tooling/simpsons/ps5-toolchain.cmake"
BUILD="${SRC}/build-ps5"

python3 "${PORT}/apply_simpsons_recomp_ps5.py" "${SRC}" --repo-root "${ROOT}"

cmake -S "${SRC}/simpsons" -B "${BUILD}" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
  -DREXSDK_DIR="${SRC}/tools/rexglue-sdk" \
  -DREXGLUE_PS5=ON \
  -DPS5RT_BUILD_PS5_BACKEND=ON \
  -DREXGLUE_USE_D3D12=OFF \
  -DREXGLUE_USE_VULKAN=ON \
  -DREXGLUE_ENABLE_TRACY=OFF \
  -DREXGLUE_ENABLE_FIDELITYFX=OFF \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo

cmake --build "${BUILD}" --target simpsons -j"${JOBS:-8}"

echo
echo "Native PS5 compile/link gate finished."
echo "Build tree: ${BUILD}"
echo "Next gate after a successful link: finalize PIE/FSELF and hardware boot."
