#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/beetle-psx-app}"
ENGINE_OUT="${ENGINE_OUT:-$ROOT/build/ps5/beetle-psx}"
SRC="$ENGINE_OUT/src/beetle-psx-libretro"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public ps5-payload SDK root}"
: "${PS5_VULKAN_DIR:?Set PS5_VULKAN_DIR to a pinned PS5_Vulkan checkout}"
: "${RADV_ARCHIVE:?Set RADV_ARCHIVE to the PS5 RADV release archive}"

CXX="${PS5_CXX:-$PS5_PAYLOAD_SDK/bin/prospero-clang++}"
CC="${PS5_CC:-$PS5_PAYLOAD_SDK/bin/prospero-clang}"
AR="${PS5_AR:-$PS5_PAYLOAD_SDK/bin/prospero-ar}"

for tool in "$CXX" "$CC" "$AR"; do
  [[ -x "$tool" ]] || { echo "missing PS5 tool: $tool" >&2; exit 2; }
done

[[ -f "$PS5_VULKAN_DIR/tools/radv-link.sh" ]] || {
  echo "missing PS5_Vulkan RADV link recipe" >&2
  exit 3
}
[[ -s "$RADV_ARCHIVE" ]] || {
  echo "missing RADV archive: $RADV_ARCHIVE" >&2
  exit 3
}

# Rebuild the exact pinned upstream engine if necessary. This script never
# accepts an interpreter-only replacement.
if [[ ! -s "$ENGINE_OUT/artifacts/libbeetle_psx_hw_ps5_lightrec.a" ]]; then
  OUT="$ENGINE_OUT" bash "$ROOT/ports/beetle-psx-ps5/build_engine_ps5.sh"
fi

ENGINE="$ENGINE_OUT/artifacts/libbeetle_psx_hw_ps5_lightrec.a"
[[ -d "$SRC/libretro-common/include" ]] || {
  echo "Beetle source tree missing; engine build must preserve its pinned source" >&2
  exit 4
}

rm -rf "$OUT"
mkdir -p "$OUT/obj" "$OUT/artifacts"

# Use the public driver's own linker contract. Do not invent a libvulkan.so
# dependency: PS5 RADV is linked into the title.
# shellcheck source=/dev/null
source "$PS5_VULKAN_DIR/tools/radv-link.sh"
radv_link_recipe "$PS5_VULKAN_DIR" "$PS5_PAYLOAD_SDK" "$RADV_ARCHIVE"

INCLUDES=(
  -I"$ROOT/runtime/include"
  -I"$ROOT/corehost/include"
  -I"$ROOT/ports/beetle-psx-ps5/native"
  -I"$SRC/libretro-common/include"
  -I"$SRC/parallel-psx/khronos/include"
)

CXXFLAGS=(
  -std=c++20 -O3 -DNDEBUG -fno-exceptions -fvisibility=hidden
  -march=znver2 -msse4.1 -mavx2 -mno-vzeroupper
  -pthread
)

SOURCES=(
  "$ROOT/runtime/src/ps5/app.cpp"
  "$ROOT/runtime/src/ps5/audio.cpp"
  "$ROOT/runtime/src/ps5/input.cpp"
  "$ROOT/runtime/src/ps5/input_hid.cpp"
  "$ROOT/runtime/src/ps5/io.cpp"
  "$ROOT/runtime/src/ps5/vfs.cpp"
  "$ROOT/runtime/src/ps5/memory.cpp"
  "$ROOT/corehost/src/static_core.cpp"
  "$ROOT/corehost/src/ps5rt_bridge.cpp"
  "$ROOT/ports/beetle-psx-ps5/native/content.cpp"
  "$ROOT/ports/beetle-psx-ps5/native/runtime_io.cpp"
  "$ROOT/ports/beetle-psx-ps5/native/vulkan_environment.cpp"
  "$ROOT/ports/beetle-psx-ps5/native/display_surface.cpp"
  "$ROOT/ports/beetle-psx-ps5/native/vulkan_provider.cpp"
  "$ROOT/ports/beetle-psx-ps5/native/vulkan_presenter.cpp"
  "$ROOT/ports/beetle-psx-ps5/native/main.cpp"
)

OBJECTS=()
for src in "${SOURCES[@]}"; do
  [[ -f "$src" ]] || { echo "missing source: $src" >&2; exit 5; }
  rel="${src#$ROOT/}"
  obj="$OUT/obj/${rel//\//_}.o"
  "$CXX" "${CXXFLAGS[@]}" "${INCLUDES[@]}" -c "$src" -o "$obj"
  OBJECTS+=("$obj")
done

PIE="$OUT/artifacts/beetle_psx_hw_ps5_pie.elf"

# radv_link_recipe exports arrays/variables owned by PS5_Vulkan:
#   radv_linker_script
#   radv_link_inputs[]
#   radv_link_flags[]
"$CXX" -o "$PIE"   "${OBJECTS[@]}"   "$ENGINE"   "${radv_link_inputs[@]}"   "${radv_link_flags[@]}"   -Wl,-T,"$radv_linker_script"   -Wl,--version-script="$ROOT/tools/ps5/app-hidden.map"   -Wl,--exclude-libs,ALL   -pthread -lm   -lSceAudioOut -lScePad -lSceUserService -lSceVideoOut -lSceSystemService

[[ -s "$PIE" ]] || { echo "PS1 native PS5 PIE missing" >&2; exit 6; }

# Architecture gates: the final title may not silently lose the recompiler.
if command -v nm >/dev/null 2>&1; then
  nm "$ENGINE" | grep -q '[[:space:]]lightrec_execute$' || {
    echo "Lightrec execute symbol missing from final engine" >&2; exit 7;
  }
  nm "$ENGINE" | grep -q '[[:space:]]_jit_set_code$' || {
    echo "GNU Lightning x86-64 emitter missing from final engine" >&2; exit 7;
  }
fi

python3 "$ROOT/tools/ps5/write_artifact_manifest.py"   --output-dir "$OUT/artifacts"   --system "Sony PlayStation"   --core "Beetle PSX HW"   --upstream "libretro/beetle-psx-libretro"   --pin "ed87921996c67658d7a70814f73034bbca08786a"   --cpu-backend "Lightrec + GNU Lightning x86-64 threaded recompiler"   --graphics-backend "Beetle Vulkan RHI -> PS5 RADV -> VK_KHR_display"   --artifact "$PIE"

find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 |
  sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"

echo "PS1 native PS5 standalone PIE complete: $PIE"
