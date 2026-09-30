#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/cemu}"
SRC="$OUT/src/Cemu"
PIN="4e3c824faa00f6b85782db019f20f29f063f3a2a"
REPO="https://github.com/cemu-project/Cemu.git"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public ps5-payload SDK root}"
CXX="${PS5_CXX:-$PS5_PAYLOAD_SDK/bin/prospero-clang++}"
AR="${PS5_AR:-$PS5_PAYLOAD_SDK/bin/prospero-ar}"

[[ -x "$CXX" ]] || { echo "missing prospero-clang++" >&2; exit 2; }
[[ -x "$AR" ]] || { echo "missing prospero-ar" >&2; exit 2; }

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/obj" "$OUT/artifacts"

git clone --filter=blob:none "$REPO" "$SRC"
git -C "$SRC" checkout --detach "$PIN"

python3 "$ROOT/tools/ps5/apply_cemu_ps5.py" "$SRC"

sources=(
  src/Cafe/HW/Espresso/Recompiler/PPCRecompiler.cpp
  src/Cafe/HW/Espresso/Recompiler/PPCRecompilerIntermediate.cpp
  src/Cafe/HW/Espresso/Recompiler/PPCRecompilerImlGen.cpp
  src/Cafe/HW/Espresso/Recompiler/PPCRecompilerImlGenFPU.cpp
  src/Cafe/HW/Espresso/Recompiler/IML/IMLSegment.cpp
  src/Cafe/HW/Espresso/Recompiler/IML/IMLInstruction.cpp
  src/Cafe/HW/Espresso/Recompiler/IML/IMLDebug.cpp
  src/Cafe/HW/Espresso/Recompiler/IML/IMLAnalyzer.cpp
  src/Cafe/HW/Espresso/Recompiler/IML/IMLOptimizer.cpp
  src/Cafe/HW/Espresso/Recompiler/IML/IMLRegisterAllocator.cpp
  src/Cafe/HW/Espresso/Recompiler/IML/IMLRegisterAllocatorRanges.cpp
  src/Cafe/HW/Espresso/Recompiler/BackendX64/BackendX64.cpp
  src/Cafe/HW/Espresso/Recompiler/BackendX64/BackendX64AVX.cpp
  src/Cafe/HW/Espresso/Recompiler/BackendX64/BackendX64BMI.cpp
  src/Cafe/HW/Espresso/Recompiler/BackendX64/BackendX64FPU.cpp
  src/Cafe/HW/Espresso/Recompiler/BackendX64/BackendX64Gen.cpp
  src/Cafe/HW/Espresso/Recompiler/BackendX64/BackendX64GenFPU.cpp
)

# Stage only header-only dependencies. Never add /usr/include itself to a
# PS5 cross-build: it would make host glibc headers outrank the PS5 sysroot.
HOST_HEADERS="$OUT/host-headers"
mkdir -p "$HOST_HEADERS"
for dep in boost fmt glm; do
  [[ -d "/usr/include/$dep" ]] || {
    echo "missing host header package: /usr/include/$dep" >&2
    exit 4
  }
  cp -a "/usr/include/$dep" "$HOST_HEADERS/$dep"
done

flags=(
  -std=c++20 -O3 -DNDEBUG -DENABLE_VULKAN=1
  -march=znver2 -msse4.1 -mavx2 -mbmi -mbmi2 -mno-vzeroupper
  -I"$SRC/src"
  -I"$SRC"
  -I"$HOST_HEADERS"
  -include "$SRC/src/Common/precompiled.h"
)

objects=()
for rel in "${sources[@]}"; do
  src="$SRC/$rel"
  obj="$OUT/obj/$(echo "$rel" | tr '/.' '__').o"
  echo "Cemu PS5 PPC x64: $rel"
  "$CXX" "${flags[@]}" -c "$src" -o "$obj"
  objects+=("$obj")
done

test "${#objects[@]}" -eq 17
"$AR" rcs "$OUT/artifacts/libcemu_ppc_x64_ps5.a" "${objects[@]}"
test -s "$OUT/artifacts/libcemu_ppc_x64_ps5.a"

contents="$("$AR" t "$OUT/artifacts/libcemu_ppc_x64_ps5.a")"
for required in BackendX64_cpp.o BackendX64AVX_cpp.o BackendX64BMI_cpp.o PPCRecompiler_cpp.o IMLOptimizer_cpp.o; do
  grep -q "$required" <<<"$contents" || {
    echo "Cemu x64 recompiler archive missing $required" >&2
    exit 3
  }
done

sha256sum "$OUT/artifacts/libcemu_ppc_x64_ps5.a" > "$OUT/artifacts/SHA256SUMS"
echo "Cemu PS5 Espresso PPC -> x86-64 recompiler archive complete"
echo "pin=$PIN"
