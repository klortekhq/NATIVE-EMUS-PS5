#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/mupen64plus}"
SRC="$OUT/src/PS5_Mupen64Plus"
PIN="98ec1019d191fc3c0e1c2d031f2574e95bd8d30d"
REPO="https://github.com/mihawk-99/PS5_Mupen64Plus.git"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public ps5-payload SDK root}"
CC="${PS5_CC:-$PS5_PAYLOAD_SDK/bin/prospero-clang}"
CXX="${PS5_CXX:-$PS5_PAYLOAD_SDK/bin/prospero-clang++}"
AR="${PS5_AR:-$PS5_PAYLOAD_SDK/bin/prospero-ar}"
RANLIB="${PS5_RANLIB:-$PS5_PAYLOAD_SDK/bin/prospero-ranlib}"

for tool in "$CC" "$CXX" "$AR" "$RANLIB"; do
  [[ -x "$tool" ]] || { echo "missing PS5 tool: $tool" >&2; exit 2; }
done

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/artifacts"

git clone --filter=blob:none "$REPO" "$SRC"
git -C "$SRC" checkout --detach "$PIN"
git -C "$SRC" submodule update --init --recursive --depth 1 || true

python3 "$ROOT/tools/ps5/apply_mupen64plus_ps5rt.py" "$SRC"

make -C "$SRC" -j"${JOBS:-2}"   platform=ps5   ARCH=x86_64   WITH_DYNAREC=x86_64   STATIC_LINKING=1   HAVE_GLIDEN64=0   HAVE_PARALLEL_RDP=0   HAVE_PARALLEL_RSP=1   HAVE_THR_AL=0   LLE=1   CC="$CC"   CC_AS="$CC"   CXX="$CXX"   AR="$AR"   RANLIB="$RANLIB"   CPPFLAGS="-I$ROOT/runtime/include"

ENGINE="$SRC/mupen64plus_next_libretro.a"
[[ -s "$ENGINE" ]] || {
  echo "Mupen64Plus PS5 static engine archive missing" >&2
  exit 3
}

CONTENTS="$("$AR" t "$ENGINE")"
grep -Eq 'new_dynarec\.c\.o$|new_dynarec\.o$' <<<"$CONTENTS" || {
  echo "R4300 x86-64 new_dynarec object missing" >&2
  exit 4
}
grep -Eq 'jit_allocator\.cpp\.o$|jit_allocator\.o$' <<<"$CONTENTS" || {
  echo "ParaLLEl-RSP PS5 JIT allocator object missing" >&2
  exit 5
}
grep -Eq 'rsp_jit\.cpp\.o$|rsp_jit\.o$' <<<"$CONTENTS" || {
  echo "ParaLLEl-RSP JIT object missing" >&2
  exit 6
}

grep -q 'ps5rt_exec_allocate'   "$SRC/mupen64plus-core/src/device/r4300/new_dynarec/new_dynarec.c" || {
  echo "R4300 dynarec allocator not routed through ps5rt" >&2
  exit 7
}
grep -q 'ps5rt_exec_allocate'   "$SRC/mupen64plus-rsp-paraLLEl/jit_allocator.cpp" || {
  echo "RSP JIT allocator not routed through ps5rt" >&2
  exit 8
}

cp "$ENGINE" "$OUT/artifacts/libmupen64plus_ps5_jit_engine.a"

python3 "$ROOT/tools/ps5/write_artifact_manifest.py"   --output-dir "$OUT/artifacts"   --system "Nintendo 64"   --core "Mupen64Plus-Next"   --upstream "mihawk-99/PS5_Mupen64Plus"   --pin "$PIN"   --cpu-backend "R4300 x86-64 new_dynarec + ParaLLEl-RSP JIT"   --graphics-backend "CPU/RSP engine gate; RDP intentionally excluded"   --artifact "libmupen64plus_ps5_jit_engine.a"

find "$OUT/artifacts" -maxdepth 1 -type f ! -name SHA256SUMS -print0 |   sort -z | xargs -0 sha256sum > "$OUT/artifacts/SHA256SUMS"

echo "Mupen64Plus PS5 R4300 + RSP JIT engine build complete"
