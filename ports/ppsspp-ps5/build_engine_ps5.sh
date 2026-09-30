#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/ppsspp-engine}"
SRC="$OUT/src/ppsspp"
PIN="fa50bb1976065c4f8b1b47af227d367fe9771555"
REPO="https://github.com/hrydgard/ppsspp.git"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public ps5-payload SDK root}"
AR="${PS5_AR:-$PS5_PAYLOAD_SDK/bin/prospero-ar}"

[[ -x "$AR" ]] || { echo "missing prospero-ar: $AR" >&2; exit 2; }

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/artifacts"

git clone --filter=blob:none "$REPO" "$SRC"
git -C "$SRC" checkout --detach "$PIN"
git -C "$SRC" submodule update --init --recursive --depth 1

python3 "$ROOT/tools/ps5/apply_ppsspp_native.py" "$SRC"

PPSSPP_SOURCE_DIR="$SRC" \
  PPSSPP_BUILD_DIR="$OUT/build" \
  PS5_PAYLOAD_SDK="$PS5_PAYLOAD_SDK" \
  "$ROOT/tools/ps5/build_ppsspp_engine_archives.sh" "$SRC"

BUILD="$OUT/build"
CORE="$(find "$BUILD" -type f -name 'libCore.a' -print -quit)"
COMMON="$(find "$BUILD" -type f -name 'libCommon.a' -print -quit)"

[[ -n "$CORE" && -s "$CORE" ]] || { echo "PPSSPP libCore.a missing" >&2; exit 3; }
[[ -n "$COMMON" && -s "$COMMON" ]] || { echo "PPSSPP libCommon.a missing" >&2; exit 4; }

CONTENTS="$("$AR" t "$CORE")"

grep -Eq '(^|/)Jit\.cpp\.o$|Jit\.cpp\.o' <<<"$CONTENTS" || {
  echo "PPSSPP x86 JIT object missing from libCore.a" >&2
  exit 5
}
grep -Eq 'X64IRJit\.cpp\.o' <<<"$CONTENTS" || {
  echo "PPSSPP X64 IR JIT object missing from libCore.a" >&2
  exit 6
}
grep -Eq 'JitBlockCache\.cpp\.o' <<<"$CONTENTS" || {
  echo "PPSSPP JIT block cache missing from libCore.a" >&2
  exit 7
}

cp "$CORE" "$OUT/artifacts/libppsspp_core_ps5.a"
cp "$COMMON" "$OUT/artifacts/libppsspp_common_ps5.a"

python3 "$ROOT/tools/ps5/write_artifact_manifest.py" \
  --output-dir "$OUT/artifacts" \
  --system "PlayStation Portable" \
  --core "PPSSPP" \
  --upstream "hrydgard/ppsspp" \
  --pin "$PIN" \
  --cpu-backend "MIPS x86-64 JIT / X64 IR JIT" \
  --graphics-backend "Vulkan/RADV target" \
  --artifact "libppsspp_core_ps5.a" \
  --artifact "libppsspp_common_ps5.a"

find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 | \
  sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"

echo "PPSSPP PS5 x86-64 JIT engine build complete"
