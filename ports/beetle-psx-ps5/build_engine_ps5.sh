#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/beetle-psx}"
SRC="$OUT/src/PS5_BeetlePSX"
PIN="e43b3980e031c47066917c941be6ace6f51ed24f"
REPO="https://github.com/mihawk-99/PS5_BeetlePSX.git"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public ps5-payload SDK root}"
CC="${PS5_CC:-$PS5_PAYLOAD_SDK/bin/prospero-clang}"
CXX="${PS5_CXX:-$PS5_PAYLOAD_SDK/bin/prospero-clang++}"
AR="${PS5_AR:-$PS5_PAYLOAD_SDK/bin/prospero-ar}"
RANLIB="${PS5_RANLIB:-$PS5_PAYLOAD_SDK/bin/prospero-ranlib}"
NM="${NM:-nm}"

for tool in "$CC" "$CXX" "$AR" "$RANLIB"; do
  [[ -x "$tool" ]] || { echo "missing PS5 tool: $tool" >&2; exit 2; }
done
command -v "$NM" >/dev/null || { echo "missing nm: $NM" >&2; exit 2; }

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/artifacts"

git clone --filter=blob:none "$REPO" "$SRC"
git -C "$SRC" checkout --detach "$PIN"

python3 "$ROOT/tools/ps5/apply_beetle_psx_ps5.py" "$SRC"

ENGINE="$OUT/artifacts/libbeetle_psx_hw_ps5_lightrec.a"

make -C "$SRC" -j"${JOBS:-2}" \
  platform=ps5 \
  STATIC_LINKING=1 \
  TARGET="$ENGINE" \
  HAVE_LIGHTREC=1 \
  THREADED_RECOMPILER=1 \
  CC="$CC" \
  CXX="$CXX" \
  AR="$AR" \
  RANLIB="$RANLIB" \
  EXTRA_FLAGS="-I$ROOT/runtime/include -march=znver2 -msse4.1 -mavx2 -mno-vzeroupper"

[[ -s "$ENGINE" ]] || {
  echo "Beetle PSX HW PS5 Lightrec archive missing" >&2
  exit 3
}

CONTENTS="$("$AR" t "$ENGINE")"

for object in lightrec.o recompiler.o lightning.o jit_memory.o; do
  grep -Eq "(^|/)$object$" <<<"$CONTENTS" || {
    echo "required Lightrec/GNU Lightning object missing: $object" >&2
    exit 4
  }
done

grep -q 'HAVE_LIGHTREC = 1' "$SRC/Makefile" || {
  echo "PS5 build does not keep Lightrec enabled" >&2
  exit 5
}

grep -q 'ps5rt_exec_allocate' "$SRC/libretro.c" || {
  echo "Lightrec code buffer is not routed through ps5rt" >&2
  exit 6
}

# Binary evidence: prove the archive actually contains the native recompiler
# and that its PS5-facing object resolves executable memory through ps5rt.
SYMBOLS="$("$NM" -A "$ENGINE")"
grep -Eq '[[:space:]]T[[:space:]]+lightrec_executepython3 "$ROOT/tools/ps5/write_artifact_manifest.py" \
  --output-dir "$OUT/artifacts" \
  --system "Sony PlayStation" \
  --core "Beetle PSX HW" \
  --upstream "mihawk-99/PS5_BeetlePSX" \
  --pin "$PIN" \
  --cpu-backend "Lightrec R3000A recompiler + GNU Lightning x86-64" \
  --graphics-backend "CPU/JIT engine gate; PS5 Vulkan donor path tracked separately" \
  --artifact "libbeetle_psx_hw_ps5_lightrec.a"

find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 | \
  sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"

echo "Beetle PSX HW PS5 Lightrec x86-64 engine build complete"
 <<<"$SYMBOLS" || {
  echo "Lightrec execution symbol missing from PS5 archive" >&2
  exit 7
}
grep -Eq '[[:space:]]T[[:space:]]+_jit_set_codepython3 "$ROOT/tools/ps5/write_artifact_manifest.py" \
  --output-dir "$OUT/artifacts" \
  --system "Sony PlayStation" \
  --core "Beetle PSX HW" \
  --upstream "mihawk-99/PS5_BeetlePSX" \
  --pin "$PIN" \
  --cpu-backend "Lightrec R3000A recompiler + GNU Lightning x86-64" \
  --graphics-backend "CPU/JIT engine gate; PS5 Vulkan donor path tracked separately" \
  --artifact "libbeetle_psx_hw_ps5_lightrec.a"

find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 | \
  sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"

echo "Beetle PSX HW PS5 Lightrec x86-64 engine build complete"
 <<<"$SYMBOLS" || {
  echo "GNU Lightning code-emission symbol missing from PS5 archive" >&2
  exit 8
}
grep -Eq '[[:space:]]U[[:space:]]+ps5rt_exec_allocatepython3 "$ROOT/tools/ps5/write_artifact_manifest.py" \
  --output-dir "$OUT/artifacts" \
  --system "Sony PlayStation" \
  --core "Beetle PSX HW" \
  --upstream "mihawk-99/PS5_BeetlePSX" \
  --pin "$PIN" \
  --cpu-backend "Lightrec R3000A recompiler + GNU Lightning x86-64" \
  --graphics-backend "CPU/JIT engine gate; PS5 Vulkan donor path tracked separately" \
  --artifact "libbeetle_psx_hw_ps5_lightrec.a"

find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 | \
  sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"

echo "Beetle PSX HW PS5 Lightrec x86-64 engine build complete"
 <<<"$SYMBOLS" || {
  echo "PS5 archive does not reference ps5rt executable allocation" >&2
  exit 9
}
grep -Eq '[[:space:]]U[[:space:]]+ps5rt_exec_releasepython3 "$ROOT/tools/ps5/write_artifact_manifest.py" \
  --output-dir "$OUT/artifacts" \
  --system "Sony PlayStation" \
  --core "Beetle PSX HW" \
  --upstream "mihawk-99/PS5_BeetlePSX" \
  --pin "$PIN" \
  --cpu-backend "Lightrec R3000A recompiler + GNU Lightning x86-64" \
  --graphics-backend "CPU/JIT engine gate; PS5 Vulkan donor path tracked separately" \
  --artifact "libbeetle_psx_hw_ps5_lightrec.a"

find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 | \
  sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"

echo "Beetle PSX HW PS5 Lightrec x86-64 engine build complete"
 <<<"$SYMBOLS" || {
  echo "PS5 archive does not reference ps5rt executable release" >&2
  exit 10
}

python3 "$ROOT/tools/ps5/write_artifact_manifest.py" \
  --output-dir "$OUT/artifacts" \
  --system "Sony PlayStation" \
  --core "Beetle PSX HW" \
  --upstream "mihawk-99/PS5_BeetlePSX" \
  --pin "$PIN" \
  --cpu-backend "Lightrec R3000A recompiler + GNU Lightning x86-64" \
  --graphics-backend "CPU/JIT engine gate; PS5 Vulkan donor path tracked separately" \
  --artifact "libbeetle_psx_hw_ps5_lightrec.a"

find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 | \
  sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"

echo "Beetle PSX HW PS5 Lightrec x86-64 engine build complete"
