#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PIE="${1:?usage: finalize_native_pie.sh <pie.elf> <output-dir>}"
OUT="${2:?usage: finalize_native_pie.sh <pie.elf> <output-dir>}"

: "${PS5_PAYLOAD_SDK:?Set PS5_PAYLOAD_SDK to the public ps5-payload SDK root}"

[[ -f "$PIE" ]] || { echo "PIE not found: $PIE" >&2; exit 2; }
[[ -d "$PS5_PAYLOAD_SDK/target/lib" ]] || {
  echo "PS5 SDK stub directory missing: $PS5_PAYLOAD_SDK/target/lib" >&2
  exit 2
}

PIN="dd44bbdc75437332ed22e3ba95126733419ef25a"
REPO="https://github.com/blackbearreloaded/ps5-native-app-boilerplate.git"
WORK="${NATIVE_TOOL_WORK:-$ROOT/build/ps5/native-finalizer}"
DONOR="$WORK/ps5-native-app-boilerplate"

mkdir -p "$WORK" "$OUT"

if [[ ! -d "$DONOR/.git" ]]; then
  git clone --filter=blob:none "$REPO" "$DONOR"
fi
git -C "$DONOR" fetch --depth 1 origin "$PIN"
git -C "$DONOR" checkout --detach "$PIN"

# The host converter is GPL tooling kept in this isolated donor checkout.
# It is not copied or linked into NATIVE-EMUS-PS5.
(
  cd "$DONOR"
  bash tools/setup-native-dependencies.sh --skip-sdk >/dev/null
  bash tools/build-host-tools.sh
)

TOOL="$DONOR/build/host/ps5-native-tool"
[[ -x "$TOOL" ]] || { echo "ps5-native-tool was not produced" >&2; exit 3; }

PS5_ELF="$OUT/eboot.elf"
FSELF="$OUT/eboot.bin"

"$TOOL" link   --in "$PIE"   --out "$PS5_ELF"   --stub-dir "$PS5_PAYLOAD_SDK/target/lib"   --file-name eboot.elf

"$TOOL" self --sign --in "$PS5_ELF" --out "$FSELF"
"$TOOL" self --inspect --file "$FSELF"

[[ -s "$PS5_ELF" && -s "$FSELF" ]] || {
  echo "native finalizer did not produce eboot.elf + eboot.bin" >&2
  exit 4
}

sha256sum "$PS5_ELF" "$FSELF" > "$OUT/NATIVE_SHA256SUMS"
echo "Native PS5 finalization complete"
echo "ps5-elf=$PS5_ELF"
echo "fself=$FSELF"
