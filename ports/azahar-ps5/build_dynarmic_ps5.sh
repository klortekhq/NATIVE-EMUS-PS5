#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/azahar-dynarmic}"
SRC="$OUT/src/dynarmic"
PIN="e77b1ba0b7da7cbe93021b01a663acfe7c4dd516"
REPO="https://github.com/azahar-emu/dynarmic.git"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public ps5-payload SDK root}"
CMAKE="${PS5_CMAKE:-$PS5_PAYLOAD_SDK/bin/prospero-cmake}"
AR="${PS5_AR:-$PS5_PAYLOAD_SDK/bin/prospero-ar}"

[[ -x "$CMAKE" ]] || { echo "missing prospero-cmake" >&2; exit 2; }
[[ -x "$AR" ]] || { echo "missing prospero-ar" >&2; exit 2; }

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/build" "$OUT/artifacts"

git clone --filter=blob:none "$REPO" "$SRC"
git -C "$SRC" checkout --detach "$PIN"
git -C "$SRC" submodule update --init --recursive --depth 1

python3 "$ROOT/tools/ps5/apply_azahar_dynarmic_ps5rt.py" "$SRC"

BOOST_SOURCE="${BOOST_INCLUDE_DIR:-/usr/include}"
if [[ ! -d "$BOOST_SOURCE/boost" ]]; then
  echo "Boost headers not found at $BOOST_SOURCE/boost; set BOOST_INCLUDE_DIR" >&2
  exit 8
fi
mkdir -p "$OUT/boost/include"
cp -a "$BOOST_SOURCE/boost" "$OUT/boost/include/boost"

"$CMAKE" -S "$SRC" -B "$OUT/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG -march=znver2 -msse4.1 -mavx2 -mno-vzeroupper -DDYNARMIC_ENABLE_NO_EXECUTE_SUPPORT -I$ROOT/runtime/include" \
  -DBoost_INCLUDE_DIR="$OUT/boost/include" \
  -DDYNARMIC_FRONTENDS="A32" \
  -DDYNARMIC_TESTS=OFF \
  -DDYNARMIC_TESTS_USE_UNICORN=OFF \
  -DDYNARMIC_USE_LLVM=OFF \
  -DDYNARMIC_USE_BUNDLED_EXTERNALS=ON \
  -DDYNARMIC_USE_PRECOMPILED_HEADERS=OFF \
  -DDYNARMIC_WARNINGS_AS_ERRORS=OFF

"$CMAKE" --build "$OUT/build" --target dynarmic --parallel "${JOBS:-2}"

ENGINE="$(find "$OUT/build" -type f -name 'libdynarmic.a' -print -quit)"
[[ -n "$ENGINE" && -s "$ENGINE" ]] || {
  echo "Azahar Dynarmic PS5 archive missing" >&2
  exit 3
}

CONTENTS="$("$AR" t "$ENGINE")"
grep -q 'block_of_code.cpp.o' <<<"$CONTENTS" || {
  echo "x64 block-of-code backend missing" >&2
  exit 4
}
grep -q 'a32_jitstate.cpp.o' <<<"$CONTENTS" || {
  echo "A32 x64 JIT backend missing" >&2
  exit 5
}
grep -q 'ps5rt_sparse_arena_create' "$SRC/src/dynarmic/backend/x64/block_of_code.cpp" || {
  echo "Azahar Dynarmic allocator was not retargeted to sparse ps5rt" >&2
  exit 6
}
grep -q 'ps5rt_sparse_arena_commit' "$SRC/src/dynarmic/backend/x64/block_of_code.cpp" || {
  echo "Azahar Dynarmic incremental commit bridge missing" >&2
  exit 7
}
grep -q 'ps5rt_sparse_arena_protect' "$SRC/src/dynarmic/backend/x64/block_of_code.cpp" || {
  echo "Azahar Dynarmic W^X bridge missing" >&2
  exit 9
}

cp "$ENGINE" "$OUT/artifacts/libazahar_dynarmic_ps5.a"
python3 "$ROOT/tools/ps5/write_artifact_manifest.py" \
  --output-dir "$OUT/artifacts" \
  --system "Nintendo 3DS" \
  --core "Azahar Dynarmic" \
  --upstream "azahar-emu/dynarmic" \
  --pin "$PIN" \
  --cpu-backend "ARMv7 A32 -> x86-64 Dynarmic JIT / sparse ps5rt arena" \
  --graphics-backend "CPU engine only" \
  --artifact "libazahar_dynarmic_ps5.a"

find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 | \
  sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"

echo "Azahar Dynarmic PS5 A32 sparse x86-64 JIT build complete"
