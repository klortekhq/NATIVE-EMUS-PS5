#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK="${WORK:-$ROOT/build/ps5/radv-stack}"
BUNDLE="${BUNDLE:-$ROOT/build/ps5/radv-bundle}"
JOBS="${JOBS:-2}"

OUT="$WORK" bash "$ROOT/tools/ps5/prepare_radv_source_stack.sh"

VULKAN="$WORK/PS5_Vulkan"
MESA="$WORK/PS5_Mesa"
SDK_FORK="$WORK/PS5_PayloadSDK"

[[ "$(git -C "$VULKAN" rev-parse HEAD)" == "71026e7ec1951fe72ae5b9118ff0905216a4c220" ]]
[[ "$(git -C "$MESA" rev-parse HEAD)" == "cedb774b27d089fa81f46add28d0a8c13ff0f7d2" ]]
[[ "$(git -C "$SDK_FORK" rev-parse HEAD)" == "95c08f27386fc698f6bbe21dde3030140a41d10b" ]]

export PS5_MESA_FORK="$MESA"
export PS5_PAYLOAD_SDK_FORK="$SDK_FORK"
export BUILD_JOBS="$JOBS"
export CMAKE_BUILD_PARALLEL_LEVEL="$JOBS"

bash "$VULKAN/tools/setup-native-dependencies.sh"
bash "$VULKAN/tools/build-radv.sh" release

RELEASE="$VULKAN/.deps/native/radv-release"
ARCHIVE="$RELEASE/lib/libvulkan_radeon.ps5.a"
STAGED_SDK="$VULKAN/.deps/native/ps5-payload-sdk"

[[ -s "$ARCHIVE" ]] || {
  echo "RADV release archive missing: $ARCHIVE" >&2
  exit 3
}
[[ -x "$STAGED_SDK/bin/prospero-clang" ]] || {
  echo "PS5_Vulkan did not stage the matching payload SDK" >&2
  exit 4
}

rm -rf "$BUNDLE"
mkdir -p   "$BUNDLE/lib"   "$BUNDLE/tools"   "$BUNDLE/tooling/native"   "$BUNDLE/vendor/ps5/sdk/stubs"

cp "$ARCHIVE" "$BUNDLE/lib/libvulkan_radeon.ps5.a"
cp "$VULKAN/tools/radv-link.sh" "$BUNDLE/tools/radv-link.sh"
cp "$VULKAN/tooling/native/app_crt.cpp" "$BUNDLE/tooling/native/app_crt.cpp"
cp "$VULKAN/tooling/native/app_cpp_runtime.cpp" "$BUNDLE/tooling/native/app_cpp_runtime.cpp"
cp "$VULKAN/vendor/ps5/sdk/stubs/agc_canary_link_stub.c"    "$BUNDLE/vendor/ps5/sdk/stubs/agc_canary_link_stub.c"
cp "$VULKAN/vendor/ps5/sdk/stubs/agc_driver_canary_link_stub.c"    "$BUNDLE/vendor/ps5/sdk/stubs/agc_driver_canary_link_stub.c"
cp -a "$STAGED_SDK" "$BUNDLE/sdk"

python3 "$ROOT/tools/ps5/freeze_radv_bundle.py" "$BUNDLE"
python3 "$ROOT/tools/ps5/validate_radv_bundle.py" "$BUNDLE"

echo "Immutable PS5 RADV bundle ready: $BUNDLE"
echo "Use: PS5_RADV_BUNDLE_DIR=$BUNDLE bash ports/beetle-psx-ps5/build_app_ps5.sh"
