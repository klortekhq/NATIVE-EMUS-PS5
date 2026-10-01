#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/genesis-plus-gx}"
SRC="$OUT/src"
CORE="$SRC/genesis-plus-gx"
PIN="c2838c7dc4236fc2fe94e5dbd08b41486067918e"
SHA="7ba2eab9d6dae71bb42e8208573300ad3475a4263e860178c4d19c36d85fc92b"
URL="https://codeload.github.com/libretro/Genesis-Plus-GX/tar.gz/$PIN"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK}"
REAL_CC="${REAL_CC:-$PS5_PAYLOAD_SDK/bin/prospero-clang}"
REAL_CXX="${REAL_CXX:-$PS5_PAYLOAD_SDK/bin/prospero-clang++}"
AR="${AR:-$PS5_PAYLOAD_SDK/bin/prospero-ar}"
for t in "$REAL_CC" "$REAL_CXX" "$AR"; do
  [[ -x "$t" ]] || { echo "missing tool: $t" >&2; exit 2; }
done

rm -rf "$OUT"
mkdir -p "$SRC" "$OUT/obj" "$OUT/artifacts" "$OUT/toolwrap"

cat > "$OUT/toolwrap/cc" <<EOF
#!/usr/bin/env bash
exec "$REAL_CC" -march=znver2 -msse4.1 -mavx2 -fno-strict-aliasing "\$@"
EOF
cat > "$OUT/toolwrap/cxx" <<EOF
#!/usr/bin/env bash
exec "$REAL_CXX" -march=znver2 -msse4.1 -mavx2 -fno-strict-aliasing "\$@"
EOF
chmod +x "$OUT/toolwrap/cc" "$OUT/toolwrap/cxx"
CC="$OUT/toolwrap/cc"
CXX="$OUT/toolwrap/cxx"

curl -fL --retry 3 "$URL" -o "$OUT/core.tar.gz"
echo "$SHA  $OUT/core.tar.gz" | sha256sum -c -
tar -xzf "$OUT/core.tar.gz" -C "$SRC"
mv "$SRC/Genesis-Plus-GX-$PIN" "$CORE"

CORE_LIB="$OUT/libgenesis_plus_gx_libretro.a"
make -C "$CORE" -f Makefile.libretro \
  platform=unix STATIC_LINKING=1 \
  TARGET="$CORE_LIB" \
  CC="$CC" CXX="$CXX" AR="$AR" \
  HAVE_CHD=0 HAVE_CDROM=0 FRONTEND_SUPPORTS_RGB565=1
[[ -s "$CORE_LIB" ]] || { echo "missing Genesis Plus GX archive" >&2; exit 3; }

# The static core intentionally leaves libretro-common frontend facilities
# to the frontend. Compile the exact vendored implementations from this pin.
LRC="$CORE/libretro/libretro-common"
LRC_SOURCES=(
  "$LRC/streams/file_stream.c"
  "$LRC/streams/file_stream_transforms.c"
  "$LRC/compat/fopen_utf8.c"
  "$LRC/compat/compat_snprintf.c"
  "$LRC/compat/compat_strl.c"
  "$LRC/compat/compat_strcasestr.c"
  "$LRC/compat/compat_posix_string.c"
  "$LRC/encodings/encoding_utf.c"
  "$LRC/file/file_path.c"
  "$LRC/file/retro_dirent.c"
  "$LRC/lists/string_list.c"
  "$LRC/lists/dir_list.c"
  "$LRC/memmap/memalign.c"
  "$LRC/string/stdstring.c"
  "$LRC/vfs/vfs_implementation.c"
)
LRC_OBJECTS=()
for src in "${LRC_SOURCES[@]}"; do
  rel="${src#$LRC/}"
  obj="$OUT/obj/lrc_${rel//\//_}.o"
  "$CC" -O2 -DNDEBUG -fPIC \
    -I"$LRC/include" -I"$CORE/libretro" -I"$CORE/core" \
    -c "$src" -o "$obj"
  LRC_OBJECTS+=("$obj")
done

HOST_SOURCES=(
  "$ROOT/runtime/src/ps5/app.cpp"
  "$ROOT/runtime/src/ps5/audio.cpp"
  "$ROOT/runtime/src/ps5/input.cpp"
  "$ROOT/runtime/src/ps5/input_hid.cpp"
  "$ROOT/runtime/src/ps5/io.cpp"
  "$ROOT/runtime/src/ps5/vfs.cpp"
  "$ROOT/corehost/src/static_core.cpp"
  "$ROOT/corehost/src/ps5rt_bridge.cpp"
  "$ROOT/corehost/src/vfs.cpp"
  "$ROOT/corehost/src/linked_core.cpp"
  "$ROOT/corehost/src/ps5_runner.cpp"
  "$ROOT/runtime/src/ps5/video.cpp"
)
HOST_OBJECTS=()
for src in "${HOST_SOURCES[@]}"; do
  rel="${src#$ROOT/}"
  obj="$OUT/obj/${rel//\//_}.o"
  "$CXX" -std=c++20 -O3 -DNDEBUG -fno-exceptions -fvisibility=hidden -pthread \
    -I"$ROOT/runtime/include" -I"$ROOT/corehost/include" \
    -c "$src" -o "$obj"
  HOST_OBJECTS+=("$obj")
done

build_variant() {
  local key="$1"
  local source="$2"
  local output="$3"
  local main_obj="$OUT/obj/main_${key}.o"
  "$CXX" -std=c++20 -O3 -DNDEBUG -fno-exceptions -fvisibility=hidden -pthread \
    -I"$ROOT/runtime/include" -I"$ROOT/corehost/include" \
    -c "$ROOT/ports/genesis-plus-gx-ps5/$source" -o "$main_obj"

  local pie="$OUT/artifacts/$output"
  "$CXX" -o "$pie" \
    "${HOST_OBJECTS[@]}" "$main_obj" "$CORE_LIB" "${LRC_OBJECTS[@]}" \
    -Wl,--version-script="$ROOT/tools/ps5/app-hidden.map" \
    -Wl,--exclude-libs,ALL \
    -pthread -lm \
    -lSceAudioOut -lScePad -lSceUserService -lSceVideoOut -lSceSystemService
  [[ -s "$pie" ]] || { echo "missing output: $pie" >&2; exit 5; }
  echo "built $pie"
}

build_variant sg1000 main_sg1000.cpp sg1000_genesis_plus_gx_pie.elf
build_variant mastersystem main_mastersystem.cpp mastersystem_genesis_plus_gx_pie.elf
build_variant gamegear main_gamegear.cpp gamegear_genesis_plus_gx_pie.elf
build_variant megadrive main_megadrive.cpp megadrive_genesis_plus_gx_pie.elf

for spec in \
  "Sega SG-1000|sg1000_genesis_plus_gx_pie.elf" \
  "Sega Master System|mastersystem_genesis_plus_gx_pie.elf" \
  "Sega Game Gear|gamegear_genesis_plus_gx_pie.elf" \
  "Mega Drive / Genesis|megadrive_genesis_plus_gx_pie.elf"; do
  IFS='|' read -r system artifact <<<"$spec"
  python3 "$ROOT/tools/ps5/write_artifact_manifest.py" \
    --output-dir "$OUT/artifacts" \
    --system "$system" \
    --core "Genesis Plus GX" \
    --upstream "libretro/Genesis-Plus-GX" \
    --pin "$PIN" \
    --cpu-backend "upstream-interpreter" \
    --graphics-backend "software -> ps5rt VideoOut" \
    --artifact "$artifact"
  manifest_base="${artifact%.elf}"
  mv "$OUT/artifacts/BUILD-MANIFEST.json" \
     "$OUT/artifacts/${manifest_base}.manifest.json"
done

find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 | \
  sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"
echo "Genesis Plus GX PS5 cartridge-family builds complete"
