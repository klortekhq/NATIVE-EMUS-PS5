#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PIE="${1:?usage: finalize_native_pie.sh <pie.elf> <output-dir> [extra-stub.so ...]}"
OUT="${2:?usage: finalize_native_pie.sh <pie.elf> <output-dir> [extra-stub.so ...]}"
shift 2

EXTRA_STUB_ARGS=()
for stub in "$@"; do
  [[ -s "$stub" ]] || { echo "extra native stub missing: $stub" >&2; exit 2; }
  EXTRA_STUB_ARGS+=(--stub "$stub")
done

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

# Seed the donor's zlib cache ourselves with a byte-verified release archive.
# The donor pins the same 1.3.2 digest, but its single zlib.net download path
# can occasionally return a transient/non-archive response in CI. Keep the
# donor source untouched and provide the exact archive it already expects.
ZLIB_VERSION="1.3.2"
ZLIB_SHA256="bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16"
ZLIB_CACHE="$DONOR/.deps/native/zlib"
ZLIB_ARCHIVE="$ZLIB_CACHE/zlib-$ZLIB_VERSION.tar.gz"

seed_verified_zlib_archive() {
  mkdir -p "$ZLIB_CACHE"

  if [[ -f "$ZLIB_ARCHIVE" ]] &&
      printf '%s  %s\n' "$ZLIB_SHA256" "$ZLIB_ARCHIVE" |
        sha256sum --check --strict >/dev/null 2>&1; then
    return 0
  fi

  rm -f "$ZLIB_ARCHIVE" "$ZLIB_ARCHIVE.download"

  local url
  for url in \
    "https://github.com/madler/zlib/releases/download/v$ZLIB_VERSION/zlib-$ZLIB_VERSION.tar.gz" \
    "https://zlib.net/fossils/zlib-$ZLIB_VERSION.tar.gz"; do
    rm -f "$ZLIB_ARCHIVE.download"
    if wget -q --tries=3 --timeout=30 "$url" -O "$ZLIB_ARCHIVE.download" &&
        printf '%s  %s\n' "$ZLIB_SHA256" "$ZLIB_ARCHIVE.download" |
          sha256sum --check --strict >/dev/null 2>&1; then
      mv "$ZLIB_ARCHIVE.download" "$ZLIB_ARCHIVE"
      return 0
    fi
  done

  rm -f "$ZLIB_ARCHIVE.download"
  echo "unable to fetch verified zlib $ZLIB_VERSION source archive" >&2
  return 1
}

seed_verified_zlib_archive

# The host converter is GPL tooling kept in this isolated donor checkout.
# It is not copied or linked into NATIVE-EMUS-PS5.
(
  cd "$DONOR"
  bash tools/setup-native-dependencies.sh --skip-sdk >/dev/null
  USE_CCACHE=0 bash tools/build-host-tools.sh
)

TOOL="$DONOR/build/host/ps5-native-tool"
[[ -x "$TOOL" ]] || { echo "ps5-native-tool was not produced" >&2; exit 3; }

PS5_ELF="$OUT/eboot.elf"
FSELF="$OUT/eboot.bin"

"$TOOL" link \
  --in "$PIE" \
  --out "$PS5_ELF" \
  --stub-dir "$PS5_PAYLOAD_SDK/target/lib" \
  "${EXTRA_STUB_ARGS[@]}" \
  --file-name eboot.elf

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
