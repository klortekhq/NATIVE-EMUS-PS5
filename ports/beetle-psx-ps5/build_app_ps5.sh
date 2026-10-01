#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/beetle-psx-app}"
ENGINE_OUT="${ENGINE_OUT:-$ROOT/build/ps5/beetle-psx}"
SRC="$ENGINE_OUT/src/beetle-psx-libretro"

RADV_SOURCE_MODE="legacy-checkout"
if [[ -n "${PS5_RADV_BUNDLE_DIR:-}" ]]; then
  PS5_RADV_BUNDLE_DIR="$(cd "$PS5_RADV_BUNDLE_DIR" && pwd)"
  python3 "$ROOT/tools/ps5/validate_radv_bundle.py" "$PS5_RADV_BUNDLE_DIR"
  PS5_VULKAN_DIR="$PS5_RADV_BUNDLE_DIR"
  RADV_ARCHIVE="$PS5_RADV_BUNDLE_DIR/lib/libvulkan_radeon.ps5.a"
  PS5_PAYLOAD_SDK="$PS5_RADV_BUNDLE_DIR/sdk"
  RADV_SOURCE_MODE="immutable-bundle"
else
  : "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK or PS5_RADV_BUNDLE_DIR}"
  : "${PS5_VULKAN_DIR:?Set PS5_VULKAN_DIR or PS5_RADV_BUNDLE_DIR}"
  : "${RADV_ARCHIVE:?Set RADV_ARCHIVE or PS5_RADV_BUNDLE_DIR}"
fi

CC="${PS5_CC:-$PS5_PAYLOAD_SDK/bin/prospero-clang}"
CXX="${PS5_CXX:-$PS5_PAYLOAD_SDK/bin/prospero-clang++}"
LLD="${PS5_LLD:-$PS5_PAYLOAD_SDK/bin/prospero-lld}"

for tool in "$CC" "$CXX" "$LLD"; do
  [[ -x "$tool" ]] || { echo "missing PS5 tool: $tool" >&2; exit 2; }
done

[[ -f "$PS5_VULKAN_DIR/tools/radv-link.sh" ]] || {
  echo "missing PS5_Vulkan RADV link recipe" >&2
  exit 3
}
[[ -f "$PS5_VULKAN_DIR/tooling/native/app_crt.cpp" ]] || {
  echo "missing PS5_Vulkan native CRT" >&2
  exit 3
}
[[ -f "$PS5_VULKAN_DIR/tooling/native/app_cpp_runtime.cpp" ]] || {
  echo "missing PS5_Vulkan native C++ runtime" >&2
  exit 3
}
[[ -s "$RADV_ARCHIVE" ]] || {
  echo "missing RADV archive: $RADV_ARCHIVE" >&2
  exit 3
}

# Rebuild the exact pinned public upstream engine if needed.
if [[ ! -s "$ENGINE_OUT/artifacts/libbeetle_psx_hw_ps5_lightrec.a" ]]; then
  OUT="$ENGINE_OUT" bash "$ROOT/ports/beetle-psx-ps5/build_engine_ps5.sh"
fi

ENGINE="$ENGINE_OUT/artifacts/libbeetle_psx_hw_ps5_lightrec.a"
[[ -d "$SRC/libretro-common/include" ]] || {
  echo "Beetle source tree missing; rebuild the engine first" >&2
  exit 4
}

rm -rf "$OUT"
mkdir -p "$OUT/obj" "$OUT/stubs" "$OUT/artifacts"

# PS5_Vulkan owns the exact RADV linker contract. This produces arrays:
#   radv_linker_script[]
#   radv_link_inputs[]
#   radv_link_flags[]
# and also emits its local-symbol map.
# shellcheck source=/dev/null
source "$PS5_VULKAN_DIR/tools/radv-link.sh"
radv_link_recipe "$PS5_VULKAN_DIR" "$PS5_PAYLOAD_SDK" "$RADV_ARCHIVE" || exit 5

INCLUDES=(
  -I"$ROOT/runtime/include"
  -I"$ROOT/corehost/include"
  -I"$ROOT/ports/beetle-psx-ps5/native"
  -I"$SRC"
  -I"$SRC/libretro-common/include"
  -I"$SRC/parallel-psx/khronos/include"
)

CXXFLAGS=(
  -std=c++20 -O3 -DNDEBUG -fno-exceptions -fno-rtti
  -ffunction-sections -fdata-sections -fvisibility=hidden
  -march=znver2 -msse4.1 -mavx2 -mno-vzeroupper
  -pthread
)

CFLAGS=(
  -std=c11 -O3 -DNDEBUG -fPIC -fvisibility=hidden
  -D__PROSPERO__ -DSTATIC_LINKING
  -DWANT_THREADING -DHAVE_THREADS
  -DSTDC_HEADERS -D_FILE_OFFSET_BITS=64
  -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
  -pthread
)

# Exact libretro-common set that Beetle excludes when STATIC_LINKING=1.
# The frontend owns these helpers; keeping the list aligned with the pinned
# Makefile.common avoids duplicate symbols and dynamic-frontend assumptions.
STATIC_SUPPORT_SOURCES=(
  "$SRC/libretro-common/streams/file_stream.c"
  "$SRC/libretro-common/streams/file_stream_transforms.c"
  "$SRC/libretro-common/file/file_path.c"
  "$SRC/libretro-common/file/file_path_io.c"
  "$SRC/libretro-common/file/retro_dirent.c"
  "$SRC/libretro-common/vfs/vfs_implementation.c"
  "$SRC/libretro-common/lists/dir_list.c"
  "$SRC/libretro-common/lists/string_list.c"
  "$SRC/libretro-common/string/stdstring.c"
  "$SRC/libretro-common/string/rstrtod.c"
  "$SRC/libretro-common/compat/compat_strl.c"
  "$SRC/libretro-common/compat/fopen_utf8.c"
  "$SRC/libretro-common/compat/compat_strcasestr.c"
  "$SRC/libretro-common/compat/compat_posix_string.c"
  "$SRC/libretro-common/encodings/encoding_utf.c"
  "$SRC/libretro-common/encodings/encoding_crc32.c"
  "$SRC/libretro-common/memmap/memalign.c"
  "$SRC/libretro-common/time/rtime.c"
  "$SRC/libretro-common/hash/lrc_hash.c"
  "$SRC/libretro-common/formats/data_transfer.c"
  "$SRC/libretro-common/memmap/memmap.c"
  "$SRC/libretro-common/file/nbio/nbio_intf.c"
  "$SRC/libretro-common/file/nbio/nbio_stdio.c"
  "$SRC/libretro-common/file/nbio/nbio_windowsmmap.c"
  "$SRC/libretro-common/file/nbio/nbio_unixmmap.c"
  "$SRC/libretro-common/rthreads/rthreads.c"
  "$SRC/libretro-common/rthreads/retro_eventcount.c"
  "$SRC/libretro-common/queues/retro_spsc.c"
  "$SRC/libretro-common/queues/retro_waitable_spsc.c"
)

SOURCES=(
  "$ROOT/runtime/src/common/io.cpp"
  "$ROOT/runtime/src/ps5/app.cpp"
  "$ROOT/runtime/src/ps5/audio.cpp"
  "$ROOT/runtime/src/ps5/input.cpp"
  "$ROOT/runtime/src/ps5/input_hid.cpp"
  "$ROOT/runtime/src/ps5/io.cpp"
  "$ROOT/runtime/src/ps5/io_emu_server.cpp"
  "$ROOT/runtime/src/ps5/log.cpp"
  "$ROOT/runtime/src/ps5/vfs.cpp"
  "$ROOT/runtime/src/ps5/memory.cpp"
  "$ROOT/corehost/src/static_core.cpp"
  "$ROOT/corehost/src/vfs.cpp"
  "$ROOT/corehost/src/ps5rt_bridge.cpp"
  "$ROOT/ports/beetle-psx-ps5/native/config.cpp"
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
  [[ -f "$src" ]] || { echo "missing source: $src" >&2; exit 6; }
  rel="${src#$ROOT/}"
  obj="$OUT/obj/${rel//\//_}.o"
  "$CXX" "${CXXFLAGS[@]}" "${INCLUDES[@]}" -c "$src" -o "$obj"
  OBJECTS+=("$obj")
done

for src in "${STATIC_SUPPORT_SOURCES[@]}"; do
  [[ -f "$src" ]] || { echo "missing static frontend helper: $src" >&2; exit 6; }
  rel="${src#$SRC/}"
  obj="$OUT/obj/libretro_common_${rel//\//_}.o"
  "$CC" "${CFLAGS[@]}" "${INCLUDES[@]}" -c "$src" -o "$obj"
  OBJECTS+=("$obj")
done

# Use the same CRT/runtime shape as the public PS5_Vulkan native title.
"$CXX" -std=c++20 -O2 -fno-exceptions -fno-rtti   -ffunction-sections -fdata-sections   -c "$PS5_VULKAN_DIR/tooling/native/app_crt.cpp"   -o "$OUT/obj/app_crt.o"
"$CXX" -std=c++20 -O2 -fno-exceptions -fno-rtti   -ffunction-sections -fdata-sections   -c "$PS5_VULKAN_DIR/tooling/native/app_cpp_runtime.cpp"   -o "$OUT/obj/app_cpp_runtime.o"

# RADV/WSI uses AGC imports on PS5. Generate only import stubs; AGC itself is
# supplied by console system modules.
stub() {
  local library=$1 source=$2
  "$CC" -std=c11 -O2 -fPIC     -c "$PS5_VULKAN_DIR/$source"     -o "$OUT/obj/${library}_stub.o"
  "$LLD" --shared -soname "${library}.prx"     -o "$OUT/stubs/${library}.so" "$OUT/obj/${library}_stub.o"
}
stub libSceAgc vendor/ps5/sdk/stubs/agc_canary_link_stub.c
stub libSceAgcDriver vendor/ps5/sdk/stubs/agc_driver_canary_link_stub.c

PIE="$OUT/artifacts/beetle_psx_hw_ps5_pie.elf"

# IMPORTANT: radv-link.sh is an LLD recipe. Do not pass these arrays to
# clang++: --whole-archive/--defsym/version scripts are linker arguments.
"$LLD" "${radv_linker_script[@]}" --eh-frame-hdr   "${radv_link_flags[@]}"   --version-script "$ROOT/tools/ps5/app-hidden.map"   --exclude-libs=ALL   -e _start -o "$PIE"   "$OUT/obj/app_crt.o"   "$OUT/obj/app_cpp_runtime.o"   "${OBJECTS[@]}"   "$ENGINE"   "$OUT/stubs/libSceAgc.so"   "$OUT/stubs/libSceAgcDriver.so"   "${radv_link_inputs[@]}"   --as-needed "$PS5_PAYLOAD_SDK"/target/lib/*.so

[[ -s "$PIE" ]] || { echo "PS1 native PS5 PIE missing" >&2; exit 7; }

# Architecture gate: final app may not silently lose the native recompiler.
if command -v nm >/dev/null 2>&1; then
  nm "$ENGINE" | grep -q '[[:space:]]lightrec_execute$' || {
    echo "Lightrec execute symbol missing from final engine" >&2; exit 8;
  }
  nm "$ENGINE" | grep -q '[[:space:]]_jit_set_code$' || {
    echo "GNU Lightning emitter missing from final engine" >&2; exit 8;
  }
fi

python3 "$ROOT/tools/ps5/write_artifact_manifest.py"   --output-dir "$OUT/artifacts"   --system "Sony PlayStation"   --core "Beetle PSX HW"   --upstream "libretro/beetle-psx-libretro"   --pin "ed87921996c67658d7a70814f73034bbca08786a"   --cpu-backend "Lightrec + GNU Lightning x86-64 threaded recompiler"   --graphics-backend "Beetle Vulkan RHI -> PS5 RADV -> VK_KHR_display"   --artifact "$PIE"

cat > "$OUT/artifacts/RADV-RECEIPT.txt" <<EOF
mode=$RADV_SOURCE_MODE
driver=$RADV_ARCHIVE
ps5_vulkan_dir=$PS5_VULKAN_DIR
sdk=$PS5_PAYLOAD_SDK
EOF
if [[ "$RADV_SOURCE_MODE" == "immutable-bundle" ]]; then
  sha256sum "$PS5_RADV_BUNDLE_DIR/manifest.json" >> "$OUT/artifacts/RADV-RECEIPT.txt"
fi

find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 |
  sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"

echo "PS1 native PS5 standalone PIE complete: $PIE"
echo "RADV source mode: $RADV_SOURCE_MODE"

# Finalize the linked PIE with the shared, pinned native-title converter.
# RADV imports two AGC stubs in addition to the public SDK stub directory.
# Set PS5_FINALIZE_NATIVE=0 only for link-only diagnostics.
if [[ "${PS5_FINALIZE_NATIVE:-1}" != "0" ]]; then
  PS5_PAYLOAD_SDK="$PS5_PAYLOAD_SDK" \
    bash "$ROOT/tools/ps5/finalize_native_pie.sh" \
      "$PIE" "$OUT/artifacts/native" \
      "$OUT/stubs/libSceAgc.so" \
      "$OUT/stubs/libSceAgcDriver.so"

  [[ -s "$OUT/artifacts/native/eboot.elf" &&
     -s "$OUT/artifacts/native/eboot.bin" ]] || {
    echo "PS1 native title finalization did not produce eboot.elf + eboot.bin" >&2
    exit 9
  }
fi
