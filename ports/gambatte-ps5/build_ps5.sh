#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/gambatte}"
CORE="$OUT/src/gambatte"
PIN="d9d6cd06382d1ced30de34d56d3609452323dab1"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public ps5-payload SDK root}"

REAL_CC="${REAL_CC:-$PS5_PAYLOAD_SDK/bin/prospero-clang}"
REAL_CXX="${REAL_CXX:-$PS5_PAYLOAD_SDK/bin/prospero-clang++}"
AR="${AR:-$PS5_PAYLOAD_SDK/bin/prospero-ar}"

for tool in "$REAL_CC" "$REAL_CXX" "$AR"; do
  [[ -x "$tool" ]] || { echo "missing tool: $tool" >&2; exit 2; }
done

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/obj" "$OUT/artifacts" "$OUT/toolwrap"
PIE_LINKER="$(bash "$ROOT/tools/ps5/prepare_native_pie_linker.sh" "$OUT/native-linker")"

git init -q "$CORE"
git -C "$CORE" remote add origin https://github.com/libretro/gambatte-libretro.git
git -C "$CORE" fetch --quiet --depth=1 origin "$PIN"
git -C "$CORE" -c advice.detachedHead=false checkout --quiet --detach FETCH_HEAD
[[ "$(git -C "$CORE" rev-parse HEAD)" == "$PIN" ]] || {
  echo "Gambatte pin mismatch" >&2
  exit 3
}

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

CORE_LIB="$OUT/libgambatte_libretro.a"
make -C "$CORE" -f Makefile.libretro \
  platform=unix STATIC_LINKING=1 HAVE_NETWORK=0 \
  TARGET="$CORE_LIB" \
  CC="$CC" CXX="$CXX" AR="$AR"

[[ -s "$CORE_LIB" ]] || { echo "Gambatte static archive missing" >&2; exit 4; }

if command -v nm >/dev/null 2>&1; then
  for sym in retro_init retro_deinit retro_load_game retro_run retro_unload_game; do
    nm "$CORE_LIB" 2>/dev/null | grep -q "[[:space:]]$sym$" || {
      echo "required core symbol missing: $sym" >&2
      exit 5
    }
  done
fi

LRC="$CORE/libgambatte/libretro-common"
LRC_SOURCES=(
  "$LRC/compat/compat_posix_string.c"
  "$LRC/compat/compat_snprintf.c"
  "$LRC/compat/compat_strcasestr.c"
  "$LRC/compat/compat_strl.c"
  "$LRC/compat/fopen_utf8.c"
  "$LRC/encodings/encoding_utf.c"
  "$LRC/file/file_path.c"
  "$LRC/file/file_path_io.c"
  "$LRC/streams/file_stream.c"
  "$LRC/streams/file_stream_transforms.c"
  "$LRC/string/stdstring.c"
  "$LRC/time/rtime.c"
  "$LRC/vfs/vfs_implementation.c"
)
LRC_OBJECTS=()
for src in "${LRC_SOURCES[@]}"; do
  rel="${src#$LRC/}"
  obj="$OUT/obj/lrc_${rel//\//_}.o"
  "$CC" -O2 -DNDEBUG -fPIC \
    -I"$LRC/include" \
    -I"$CORE/libgambatte/include" \
    -I"$CORE/libgambatte/src" \
    -c "$src" -o "$obj"
  LRC_OBJECTS+=("$obj")
done

HOST_COMMON=(
  "$ROOT/runtime/src/common/io.cpp"
  "$ROOT/runtime/src/common/lifecycle.cpp"
  "$ROOT/runtime/src/ps5/app.cpp"
  "$ROOT/runtime/src/ps5/audio.cpp"
  "$ROOT/runtime/src/ps5/input.cpp"
  "$ROOT/runtime/src/ps5/input_hid.cpp"
  "$ROOT/runtime/src/ps5/io.cpp"
  "$ROOT/runtime/src/ps5/vfs.cpp"
  "$ROOT/runtime/src/ps5/video.cpp"
  "$ROOT/corehost/src/static_core.cpp"
  "$ROOT/corehost/src/ps5rt_bridge.cpp"
  "$ROOT/corehost/src/vfs.cpp"
  "$ROOT/corehost/src/linked_core.cpp"
  "$ROOT/corehost/src/ps5_runner.cpp"
)
COMMON_OBJECTS=()
for src in "${HOST_COMMON[@]}"; do
  rel="${src#$ROOT/}"
  obj="$OUT/obj/${rel//\//_}.o"
  "$CXX" -std=c++20 -O3 -DNDEBUG -fno-exceptions -fvisibility=hidden -pthread \
    -I"$ROOT/runtime/include" -I"$ROOT/corehost/include" \
    -c "$src" -o "$obj"
  COMMON_OBJECTS+=("$obj")
done

build_title() {
  local id="$1"
  local main="$2"
  local system="$3"
  local obj="$OUT/obj/${id}_main.o"

  "$CXX" -std=c++20 -O3 -DNDEBUG -fno-exceptions -fvisibility=hidden -pthread \
    -I"$ROOT/runtime/include" -I"$ROOT/corehost/include" \
    -c "$main" -o "$obj"

  local pie="$OUT/artifacts/${id}_gambatte_pie.elf"
  "$CXX" -o "$pie" "${COMMON_OBJECTS[@]}" "$obj" "$CORE_LIB" "${LRC_OBJECTS[@]}" \
    -Wl,-z,max-page-size=0x4000 \
    -Wl,--hash-style=gnu \
    -Wl,--eh-frame-hdr \
    -Wl,-T,"$PIE_LINKER" \
    -Wl,--version-script="$ROOT/tools/ps5/app-hidden.map" \
    -Wl,--exclude-libs,ALL \
    -pthread -lm \
    -lSceAudioOut -lScePad -lSceUserService -lSceVideoOut -lSceSystemService

  [[ -s "$pie" ]] || { echo "missing $id PIE" >&2; exit 6; }

  mkdir -p "$OUT/artifacts/$id-manifest"
  python3 "$ROOT/tools/ps5/write_artifact_manifest.py" \
    --output-dir "$OUT/artifacts/$id-manifest" \
    --system "$system" \
    --core "Gambatte" \
    --upstream "libretro/gambatte-libretro" \
    --pin "$PIN" \
    --cpu-backend "upstream-interpreter" \
    --graphics-backend "software -> ps5rt VideoOut" \
    --artifact "$pie"
}

build_title gb "$ROOT/ports/gambatte-ps5/main_gb.cpp" "Nintendo Game Boy"
build_title gbc "$ROOT/ports/gambatte-ps5/main_gbc.cpp" "Nintendo Game Boy Color"

find "$OUT/artifacts" -type f ! -name SHA256SUMS -print0 | sort -z | \
  xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"

echo "Gambatte PS5 native PIE cross-builds complete."
