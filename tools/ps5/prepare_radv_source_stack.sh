#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/radv-stack}"

PS5_VULKAN_REPO="https://github.com/mihawk-99/PS5_Vulkan.git"
PS5_VULKAN_PIN="71026e7ec1951fe72ae5b9118ff0905216a4c220"

PS5_MESA_REPO="https://github.com/mihawk-99/PS5_Mesa.git"
PS5_MESA_PIN="cedb774b27d089fa81f46add28d0a8c13ff0f7d2"

PS5_SDK_REPO="https://github.com/mihawk-99/PS5_PayloadSDK.git"
PS5_SDK_PIN="95c08f27386fc698f6bbe21dde3030140a41d10b"

clone_pin() {
  local url=$1
  local pin=$2
  local dir=$3
  rm -rf "$dir"
  git clone --filter=blob:none "$url" "$dir"
  git -C "$dir" checkout --detach "$pin"
  local actual
  actual="$(git -C "$dir" rev-parse HEAD)"
  [[ "$actual" == "$pin" ]] || {
    echo "pin mismatch for $url: $actual != $pin" >&2
    exit 2
  }
}

mkdir -p "$OUT"
clone_pin "$PS5_VULKAN_REPO" "$PS5_VULKAN_PIN" "$OUT/PS5_Vulkan"
clone_pin "$PS5_MESA_REPO" "$PS5_MESA_PIN" "$OUT/PS5_Mesa"
clone_pin "$PS5_SDK_REPO" "$PS5_SDK_PIN" "$OUT/PS5_PayloadSDK"

cat > "$OUT/PINS.txt" <<EOF
PS5_Vulkan=$PS5_VULKAN_PIN
PS5_Mesa=$PS5_MESA_PIN
PS5_PayloadSDK=$PS5_SDK_PIN
EOF

echo "Pinned RADV source stack prepared at: $OUT"
echo "This script fetches and verifies source revisions only."
echo "Build RADV with the pinned PS5_Vulkan tooling; do not follow repository main branches."
