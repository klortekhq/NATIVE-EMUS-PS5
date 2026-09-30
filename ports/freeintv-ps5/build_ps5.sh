#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/freeintv}"
CORE="$OUT/src/core"
PIN="ef3e0fe322bec62a7f916c0bb0834c08c348d0b4"
REPO="https://github.com/libretro/FreeIntv.git"

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

CORE_LIB="$OUT/freeintv_libretro.a"
make -C "$CORE" platform=unix STATIC_LINKING=1 \
  TARGET="$CORE_LIB" \
  CC="$CC" CXX="$CXX" AR="$AR" \
  -j"${JOBS:-2}"

[[ -s "$CORE_LIB" ]] || { echo "FreeIntv static archive was not produced" >&2; exit 3; }

# STATIC_LINKING deliberately excludes frontend-side libretro-common helpers.
# Build the exact vendored helpers from the same pinned FreeIntv checkout.
LRC="$CORE/src/deps/libretro-common"
SUPPORT_SOURCES=(
  "$LRC/file/file_path.c"
  "$LRC/file/file_path_io.c"
  "$LRC/compat/compat_posix_string.c"
  "$LRC/compat/compat_snprintf.c"
  "$LRC/compat/compat_strl.c"
  "$LRC/compat/compat_strcasestr.c"
  "$LRC/compat/fopen_utf8.c"
  "$LRC/encodings/encoding_utf.c"
  "$LRC/string/stdstring.c"
  "$LRC/streams/file_stream.c"
  "$LRC/time/rtime.c"
  "$LRC/vfs/vfs_implementation.c"
)
SUPPORT_OBJECTS=()
for src in "${SUPPORT_SOURCES[@]}"; do
  rel="${src#$LRC/}"
  obj="$OUT/obj/freeintv_lrc_${rel//\//_}.o"
  "$CC" -O2 -DNDEBUG -fPIC \
    -I"$LRC/include" -I"$CORE/src" \
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
  "$ROOT/runtime/src/ps5/app.cpp"
  "$ROOT/runtime/src/ps5/audio.cpp"
  "$ROOT/runtime/src/ps5/input.cpp"
  "$ROOT/runtime/src/ps5/io.cpp"
  "$ROOT/runtime/src/ps5/vfs.cpp"
  "$ROOT/runtime/src/ps5/video.cpp"
  "$ROOT/corehost/src/static_core.cpp"
  "$ROOT/corehost/src/ps5rt_bridge.cpp"
  "$ROOT/corehost/src/linked_core.cpp"
  "$ROOT/corehost/src/ps5_runner.cpp"
  "$ROOT/ports/freeintv-ps5/main.cpp"
)

OBJECTS=()
for src in "${SOURCES[@]}"; do
  rel="${src#$ROOT/}"
  obj="$OUT/obj/${rel//\//_}.o"
  "$CXX" -std=c++20 -O3 -DNDEBUG -fno-exceptions -pthread \
    -I"$ROOT/runtime/include" -I"$ROOT/corehost/include" \
    -c "$src" -o "$obj"
  OBJECTS+=("$obj")
done

PIE="$OUT/artifacts/freeintv_pie.elf"
"$CXX" -o "$PIE" "${OBJECTS[@]}" "$CORE_LIB" "${SUPPORT_OBJECTS[@]}" \
  -pthread -lm \
  -lSceAudioOut -lScePad -lSceUserService -lSceVideoOut -lSceSystemService

[[ -s "$PIE" ]] || { echo "native PS5 PIE missing" >&2; exit 5; }

if [[ -n "${PS5_NATIVE_TOOL:-}" ]]; then
  [[ -x "$PS5_NATIVE_TOOL" ]] || { echo "PS5_NATIVE_TOOL is not executable" >&2; exit 6; }
  "$PS5_NATIVE_TOOL" link "$PIE" "$OUT/artifacts/app_ps5.elf"
  "$PS5_NATIVE_TOOL" self "$OUT/artifacts/app_ps5.elf" "$OUT/artifacts/eboot.bin"
  [[ -s "$OUT/artifacts/app_ps5.elf" && -s "$OUT/artifacts/eboot.bin" ]] || exit 7
fi

find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 | \
  sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"

echo "FreeIntv PS5 build complete: $PIE"
