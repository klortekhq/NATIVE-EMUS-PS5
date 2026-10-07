#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/pcsx2}"
SRC="$OUT/src/PS5SX2"
PIN="9a86c58f84d9fa328589fb8e14403bbcd01d35a6"
REPO="https://github.com/Swordpdf/PS5SX2.git"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public ps5-payload SDK root}"
CXX="${PS5_CXX:-$PS5_PAYLOAD_SDK/bin/prospero-clang++}"
AR="${PS5_AR:-$PS5_PAYLOAD_SDK/bin/prospero-ar}"

[[ -x "$CXX" ]] || { echo "missing prospero-clang++" >&2; exit 2; }
[[ -x "$AR" ]] || { echo "missing prospero-ar" >&2; exit 2; }

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/artifacts"

git clone --filter=blob:none "$REPO" "$SRC"
git -C "$SRC" checkout --detach "$PIN"

cd "$SRC/ps5/coreorbis"

# Build precisely the native x86 recompilers from Swordpdf's PS5SX2 makefile.
# No renderer, frontend, BIOS, game data or non-public PS5HB_Vulkan dependency
# is needed for this CPU-engine gate.
targets=(
  obj-vk/pcsx2/x86/BaseblockEx.o
  obj-vk/pcsx2/x86/iCOP0.o
  obj-vk/pcsx2/x86/iCore.o
  obj-vk/pcsx2/x86/iFPU.o
  obj-vk/pcsx2/x86/iFPUd.o
  obj-vk/pcsx2/x86/iMMI.o
  obj-vk/pcsx2/x86/iR3000A.o
  obj-vk/pcsx2/x86/iR3000Atables.o
  obj-vk/pcsx2/x86/iR5900Analysis.o
  obj-vk/pcsx2/x86/iR5900Misc.o
  obj-vk/pcsx2/x86/ix86-32/iCore.o
  obj-vk/pcsx2/x86/ix86-32/iR5900.o
  obj-vk/pcsx2/x86/ix86-32/iR5900Arit.o
  obj-vk/pcsx2/x86/ix86-32/iR5900AritImm.o
  obj-vk/pcsx2/x86/ix86-32/iR5900Branch.o
  obj-vk/pcsx2/x86/ix86-32/iR5900Jump.o
  obj-vk/pcsx2/x86/ix86-32/iR5900LoadStore.o
  obj-vk/pcsx2/x86/ix86-32/iR5900Move.o
  obj-vk/pcsx2/x86/ix86-32/iR5900MultDiv.o
  obj-vk/pcsx2/x86/ix86-32/iR5900Shift.o
  obj-vk/pcsx2/x86/ix86-32/iR5900Templates.o
  obj-vk/pcsx2/x86/ix86-32/recVTLB.o
  obj-vk/pcsx2/x86/Vif_Dynarec.o
  obj-vk/pcsx2/x86/Vif_UnpackSSE.o
  obj-vk/pcsx2/x86/microVU.o
)

make -f Makefile.vk -j"${JOBS:-2}"   PS5_PAYLOAD_SDK="$PS5_PAYLOAD_SDK"   PS5SX2_DEPS="$OUT/unused-deps"   "${targets[@]}"

objects=()
for target in "${targets[@]}"; do
  [[ -s "$target" ]] || { echo "missing recompiler object: $target" >&2; exit 3; }
  objects+=("$target")
done

"$AR" rcs "$OUT/artifacts/libpcsx2_ps5_recompilers.a" "${objects[@]}"
test -s "$OUT/artifacts/libpcsx2_ps5_recompilers.a"

contents="$("$AR" t "$OUT/artifacts/libpcsx2_ps5_recompilers.a")"
for required in iR5900.o iR3000A.o Vif_Dynarec.o microVU.o recVTLB.o; do
  grep -q "$required" <<<"$contents" || {
    echo "PCSX2 native recompiler archive missing $required" >&2
    exit 4
  }
done

sha256sum "$OUT/artifacts/libpcsx2_ps5_recompilers.a" > "$OUT/artifacts/SHA256SUMS"
echo "PCSX2 PS5 native EE/IOP/VIF/microVU x86-64 recompiler archive complete"
echo "pin=$PIN"
