#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/nes-fceumm}"
SRC="$OUT/src"
CORE="$SRC/libretro-fceumm"
PIN="236ccdfc911e84c60fea6b9d0699c2d440a8de14"
TARBALL_SHA256="dd002cde9b5271979e0394bb9e696bd37e149ced473ff1e3629cc7fed502381f"
URL="https://codeload.github.com/libretro/libretro-fceumm/tar.gz/$PIN"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public ps5-payload SDK root}"

REAL_CC="${REAL_CC:-$PS5_PAYLOAD_SDK/bin/prospero-clang}"
REAL_CXX="${REAL_CXX:-$PS5_PAYLOAD_SDK/bin/prospero-clang++}"
AR="${AR:-$PS5_PAYLOAD_SDK/bin/prospero-ar}"

for tool in "$REAL_CC" "$REAL_CXX" "$AR"; do
  [[ -x "$tool" ]] || { echo "missing tool: $tool" >&2; exit 2; }
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

curl -fL --retry 3 "$URL" -o "$OUT/fceumm.tar.gz"
echo "$TARBALL_SHA256  $OUT/fceumm.tar.gz" | sha256sum -c -
tar -xzf "$OUT/fceumm.tar.gz" -C "$SRC"
mv "$SRC/libretro-fceumm-$PIN" "$CORE"

CORE_LIB="$OUT/libfceumm_libretro.a"
make -C "$CORE" -f Makefile.libretro \
  platform=unix STATIC_LINKING=1 \
  TARGET="$CORE_LIB" \
  CC="$CC" CXX="$CXX" AR="$AR" \
  HAVE_HDPACK=0 HAVE_NTSC=1 WANT_32BPP=1

[[ -s "$CORE_LIB" ]] || { echo "FCEUmm static archive was not produced" >&2; exit 3; }

# FCEUmm deliberately excludes these libretro-common sources when
# STATIC_LINKING=1 because the frontend is expected to provide them.
# We use the exact vendored copy from the pinned core checkout.
LRC="$CORE/src/drivers/libretro/libretro-common"
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
    -I"$CORE/src/drivers/libretro" \
    -I"$CORE/src" \
    -c "$src" -o "$obj"
  LRC_OBJECTS+=("$obj")
done

if command -v nm >/dev/null 2>&1; then
  for sym in retro_init retro_deinit retro_load_game retro_run retro_unload_game; do
    nm "$CORE_LIB" 2>/dev/null | grep -q "[[:space:]]$sym$" || {
      echo "required core symbol missing: $sym" >&2
      exit 4
    }
  done
fi

COMMON_FLAGS=(
  -std=c++20 -O3 -DNDEBUG -fno-exceptions -pthread
  -I"$ROOT/runtime/include"
  -I"$ROOT/third_party/libretro"
  -I"$ROOT/ports/common/libretro-static"
)

SOURCES=(
  "$ROOT/runtime/src/ps5/app.cpp"
  "$ROOT/runtime/src/ps5/audio.cpp"
  "$ROOT/runtime/src/ps5/input.cpp"
  "$ROOT/runtime/src/ps5/io.cpp"
  "$ROOT/runtime/src/ps5/video.cpp"
  "$ROOT/ports/common/libretro-static/native_libretro_host.cpp"
  "$ROOT/ports/common/libretro-static/linked_core.cpp"
  "$ROOT/ports/nes-fceumm/main.cpp"
)

OBJECTS=()
for src in "${SOURCES[@]}"; do
  rel="${src#$ROOT/}"
  obj="$OUT/obj/${rel//\//_}.o"
  "$CXX" "${COMMON_FLAGS[@]}" -c "$src" -o "$obj"
  OBJECTS+=("$obj")
done

PIE="$OUT/artifacts/nes_fceumm_pie.elf"
"$CXX" -o "$PIE" "${OBJECTS[@]}" "$CORE_LIB" "${LRC_OBJECTS[@]}" \
  -pthread -lm \
  -lSceAudioOut -lScePad -lSceUserService -lSceVideoOut -lSceSystemService

[[ -s "$PIE" ]] || { echo "native PIE ELF missing" >&2; exit 5; }
echo "PIE ELF: $PIE"

if [[ -n "${PS5_NATIVE_TOOL:-}" ]]; then
  [[ -x "$PS5_NATIVE_TOOL" ]] || {
    echo "PS5_NATIVE_TOOL is set but is not executable: $PS5_NATIVE_TOOL" >&2
    exit 6
  }
  NATIVE_ELF="$OUT/artifacts/app_ps5.elf"
  EBOOT="$OUT/artifacts/eboot.bin"
  "$PS5_NATIVE_TOOL" link "$PIE" "$NATIVE_ELF"
  "$PS5_NATIVE_TOOL" self "$NATIVE_ELF" "$EBOOT"
  [[ -s "$NATIVE_ELF" && -s "$EBOOT" ]] || {
    echo "native title conversion did not produce expected files" >&2
    exit 7
  }
  echo "native ELF: $NATIVE_ELF"
  echo "FSELF/eboot: $EBOOT"
else
  echo "PS5_NATIVE_TOOL not set; linked PS5 PIE is ready for title conversion."
fi

find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 | \
  sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"
echo "done"
