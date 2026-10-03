#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/o2em}"
CORE="$OUT/src/core"
PIN="679d6fec04963f6e70a7ec217e3d0ebb1fe472fc"
REPO="https://github.com/libretro/libretro-o2em.git"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public ps5-payload SDK root}"

REAL_CC="${REAL_CC:-$PS5_PAYLOAD_SDK/bin/prospero-clang}"
REAL_CXX="${REAL_CXX:-$PS5_PAYLOAD_SDK/bin/prospero-clang++}"
AR="${AR:-$PS5_PAYLOAD_SDK/bin/prospero-ar}"

for tool in "$REAL_CC" "$REAL_CXX" "$AR" git make; do
  if [[ "$tool" == "git" || "$tool" == "make" ]]; then
    command -v "$tool" >/dev/null 2>&1 || { echo "missing tool: $tool" >&2; exit 2; }
  else
    [[ -x "$tool" ]] || { echo "missing tool: $tool" >&2; exit 2; }
  fi
done

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/obj" "$OUT/artifacts" "$OUT/toolwrap"

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

git clone --filter=blob:none "$REPO" "$CORE"
git -C "$CORE" checkout --detach "$PIN"

CORE_LIB="$OUT/o2em_libretro.a"
make -C "$CORE" platform=unix STATIC_LINKING=1 \
  TARGET="$CORE_LIB" \
  CC="$CC" CXX="$CXX" AR="$AR" \
  -j"${JOBS:-2}"

[[ -s "$CORE_LIB" ]] || { echo "O2EM static archive was not produced" >&2; exit 3; }

# STATIC_LINKING excludes libretro-common frontend helpers. O2EM also uses
# its vendored WAV/resampler path for The Voice support, so compile the exact
# support set from the pinned checkout rather than disabling functionality.
LRC="$CORE/libretro-common"
SUPPORT_SOURCES=(
  "$LRC/compat/compat_posix_string.c"
  "$LRC/compat/compat_snprintf.c"
  "$LRC/compat/compat_strcasestr.c"
  "$LRC/compat/compat_strl.c"
  "$LRC/compat/fopen_utf8.c"
  "$LRC/encodings/encoding_crc32.c"
  "$LRC/encodings/encoding_utf.c"
  "$LRC/file/file_path.c"
  "$LRC/file/file_path_io.c"
  "$LRC/streams/file_stream.c"
  "$LRC/time/rtime.c"
  "$LRC/vfs/vfs_implementation.c"
  "$LRC/audio/conversion/float_to_s16.c"
  "$LRC/audio/conversion/s16_to_float.c"
  "$LRC/audio/resampler/audio_resampler.c"
  "$LRC/audio/resampler/drivers/sinc_resampler.c"
  "$LRC/features/features_cpu.c"
  "$LRC/file/config_file.c"
  "$LRC/file/config_file_userdata.c"
  "$LRC/formats/wav/rwav.c"
  "$LRC/memmap/memalign.c"
)
SUPPORT_OBJECTS=()
for src in "${SUPPORT_SOURCES[@]}"; do
  rel="${src#$LRC/}"
  obj="$OUT/obj/o2em_lrc_${rel//\//_}.o"
  "$CC" -O2 -DNDEBUG -fPIC \
    -I"$LRC/include" -I"$CORE" -I"$CORE/src" \
    -c "$src" -o "$obj"
  SUPPORT_OBJECTS+=("$obj")
done

if command -v nm >/dev/null 2>&1; then
  for sym in retro_init retro_deinit retro_get_system_info retro_get_system_av_info retro_load_game retro_run retro_unload_game; do
    nm "$CORE_LIB" 2>/dev/null | grep -q "[[:space:]]$sym$" || {
      echo "required core symbol missing: $sym" >&2
      exit 4
    }
  done
fi

SOURCES=(
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
  "$ROOT/ports/o2em-ps5/main.cpp"
)

OBJECTS=()
for src in "${SOURCES[@]}"; do
  rel="${src#$ROOT/}"
  obj="$OUT/obj/${rel//\//_}.o"
  "$CXX" -std=c++20 -O3 -DNDEBUG -fno-exceptions -fvisibility=hidden -pthread \
    -I"$ROOT/runtime/include" -I"$ROOT/corehost/include" \
    -c "$src" -o "$obj"
  OBJECTS+=("$obj")
done

PIE="$OUT/artifacts/o2em_pie.elf"
"$CXX" -o "$PIE" "${OBJECTS[@]}" "$CORE_LIB" "${SUPPORT_OBJECTS[@]}" \
  -Wl,--version-script="$ROOT/tools/ps5/app-hidden.map" \
  -Wl,--exclude-libs,ALL \
  -pthread -lm \
  -lSceAudioOut -lScePad -lSceUserService -lSceVideoOut -lSceSystemService

[[ -s "$PIE" ]] || { echo "native PS5 PIE missing" >&2; exit 5; }

if [[ -n "${PS5_NATIVE_TOOL:-}" ]]; then
  [[ -x "$PS5_NATIVE_TOOL" ]] || { echo "PS5_NATIVE_TOOL is not executable" >&2; exit 6; }
  "$PS5_NATIVE_TOOL" link "$PIE" "$OUT/artifacts/app_ps5.elf"
  "$PS5_NATIVE_TOOL" self "$OUT/artifacts/app_ps5.elf" "$OUT/artifacts/eboot.bin"
  [[ -s "$OUT/artifacts/app_ps5.elf" && -s "$OUT/artifacts/eboot.bin" ]] || exit 7
fi

python3 "$ROOT/tools/ps5/write_artifact_manifest.py" \
  --output-dir "$OUT/artifacts" \
  --system "Magnavox Odyssey2 / Philips Videopac" \
  --core "O2EM" \
  --upstream "libretro/libretro-o2em" \
  --pin "$PIN" \
  --cpu-backend "upstream-interpreter" \
  --graphics-backend "software -> ps5rt VideoOut" \
  --artifact "$PIE"

find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 | \
  sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"

echo "O2EM PS5 build complete: $PIE"
