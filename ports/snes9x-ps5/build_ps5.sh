#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/snes9x}"
SRC="$OUT/src"
CORE="$SRC/snes9x"
PIN="fae2fea08f74180759ef540ee94259213f503480"
SHA="0d4b0c4181d66668ec0040eb17f90a559f7b7fa6cbd9198593c3bdbe79acd029"
URL="https://codeload.github.com/libretro/snes9x/tar.gz/$PIN"
: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK}"
REAL_CC="${REAL_CC:-$PS5_PAYLOAD_SDK/bin/prospero-clang}"
REAL_CXX="${REAL_CXX:-$PS5_PAYLOAD_SDK/bin/prospero-clang++}"
AR="${AR:-$PS5_PAYLOAD_SDK/bin/prospero-ar}"
for t in "$REAL_CC" "$REAL_CXX" "$AR"; do [[ -x "$t" ]] || { echo "missing $t" >&2; exit 2; }; done
rm -rf "$OUT"; mkdir -p "$SRC" "$OUT/obj" "$OUT/artifacts" "$OUT/toolwrap"
cat > "$OUT/toolwrap/cc" <<EOF
#!/usr/bin/env bash
exec "$REAL_CC" -march=znver2 -msse4.1 -mavx2 -fno-strict-aliasing "\$@"
EOF
cat > "$OUT/toolwrap/cxx" <<EOF
#!/usr/bin/env bash
exec "$REAL_CXX" -march=znver2 -msse4.1 -mavx2 -fno-strict-aliasing "\$@"
EOF
chmod +x "$OUT/toolwrap/cc" "$OUT/toolwrap/cxx"
CC="$OUT/toolwrap/cc"; CXX="$OUT/toolwrap/cxx"
curl -fL --retry 3 "$URL" -o "$OUT/core.tar.gz"
echo "$SHA  $OUT/core.tar.gz" | sha256sum -c -
tar -xzf "$OUT/core.tar.gz" -C "$SRC"
mv "$SRC/snes9x-$PIN" "$CORE"
CORE_LIB="$OUT/libsnes9x_libretro.a"
make -C "$CORE/libretro" -f Makefile \
  platform=unix STATIC_LINKING=1 STATIC_LINKING_LINK=1 \
  TARGET="$CORE_LIB" CC="$CC" CXX="$CXX" AR="$AR"
[[ -s "$CORE_LIB" ]] || { echo "missing Snes9x archive" >&2; exit 3; }
LRC="$CORE/libretro/libretro-common"
LRC_SOURCES=(
 "$LRC/compat/compat_posix_string.c"
 "$LRC/compat/compat_strcasestr.c"
 "$LRC/compat/compat_snprintf.c"
 "$LRC/compat/compat_strl.c"
 "$LRC/compat/fopen_utf8.c"
 "$LRC/encodings/encoding_utf.c"
 "$LRC/encodings/encoding_deflate.c"
 "$LRC/file/file_path.c"
 "$LRC/file/file_path_io.c"
 "$LRC/streams/file_stream.c"
 "$LRC/streams/file_stream_transforms.c"
 "$LRC/string/stdstring.c"
 "$LRC/time/rtime.c"
 "$LRC/vfs/vfs_implementation.c"
 "$LRC/vfs/vfs_hybrid.c"
 "$LRC/file/retro_dirent.c"
)
LRC_OBJECTS=()
for src in "${LRC_SOURCES[@]}"; do
 rel="${src#$LRC/}"; obj="$OUT/obj/lrc_${rel//\//_}.o"
 "$CC" -O2 -DNDEBUG -fPIC -I"$LRC/include" -I"$CORE/libretro" -I"$CORE" -c "$src" -o "$obj"
 LRC_OBJECTS+=("$obj")
done
HOST_SOURCES=(
 "$ROOT/runtime/src/ps5/app.cpp"
 "$ROOT/runtime/src/ps5/audio.cpp"
 "$ROOT/runtime/src/ps5/input.cpp"
 "$ROOT/runtime/src/ps5/io.cpp"
  "$ROOT/runtime/src/ps5/vfs.cpp"
  "$ROOT/corehost/src/static_core.cpp"
  "$ROOT/corehost/src/ps5rt_bridge.cpp"
  "$ROOT/corehost/src/linked_core.cpp"
  "$ROOT/corehost/src/ps5_runner.cpp"
 "$ROOT/runtime/src/ps5/video.cpp"
 "$ROOT/ports/snes9x-ps5/main.cpp"
)
HOST_OBJECTS=()
for src in "${HOST_SOURCES[@]}"; do
 rel="${src#$ROOT/}"; obj="$OUT/obj/${rel//\//_}.o"
 "$CXX" -std=c++20 -O3 -DNDEBUG -fno-exceptions -pthread \
   -I"$ROOT/runtime/include" -I"$ROOT/corehost/include" \
   -c "$src" -o "$obj"
 HOST_OBJECTS+=("$obj")
done
PIE="$OUT/artifacts/snes9x_pie.elf"
"$CXX" -o "$PIE" "${HOST_OBJECTS[@]}" "$CORE_LIB" "${LRC_OBJECTS[@]}" \
 -pthread -lm -lSceAudioOut -lScePad -lSceUserService -lSceVideoOut -lSceSystemService
[[ -s "$PIE" ]] || exit 5
if [[ -n "${PS5_NATIVE_TOOL:-}" ]]; then
 "$PS5_NATIVE_TOOL" link "$PIE" "$OUT/artifacts/app_ps5.elf"
 "$PS5_NATIVE_TOOL" self "$OUT/artifacts/app_ps5.elf" "$OUT/artifacts/eboot.bin"
fi
python3 "$ROOT/tools/ps5/write_artifact_manifest.py" \
 --output-dir "$OUT/artifacts" \
 --system "Super Nintendo / Super Famicom" \
 --core "Snes9x" \
 --upstream "libretro/snes9x" \
 --pin "$PIN" \
 --cpu-backend "upstream-interpreter" \
 --graphics-backend "software -> ps5rt VideoOut" \
 --artifact "$PIE"
find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"
echo "SNES PS5 build complete: $PIE"
