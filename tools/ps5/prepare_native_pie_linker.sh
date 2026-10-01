#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${1:-$ROOT/build/ps5/native-linker}"
PIN="dd44bbdc75437332ed22e3ba95126733419ef25a"
SHA256="fbc52518761b58b703a2225c847c5f44a81523cd0e751755adf890ae0f0dd74c"
URL="https://raw.githubusercontent.com/BlackBearReloaded/ps5-native-app-boilerplate/$PIN/tooling/native/ps5-pie.ld"
SOURCE="$OUT/ps5-pie.blackbear.ld"
SCRIPT="$OUT/ps5-pie-sdk.ld"

mkdir -p "$OUT"

if [[ ! -s "$SOURCE" ]] || ! echo "$SHA256  $SOURCE" | sha256sum -c - >/dev/null 2>&1; then
  tmp="$SOURCE.tmp"
  rm -f "$tmp"
  curl --fail --location --retry 3 --retry-all-errors "$URL" -o "$tmp"
  echo "$SHA256  $tmp" | sha256sum -c - >&2
  mv "$tmp" "$SOURCE"
fi

echo "$SHA256  $SOURCE" | sha256sum -c - >&2

# BlackBear's native converter requires its page-separated PT_LOAD layout.
# ps5-payload-dev/sdk additionally references the EH-frame and BSS boundary
# symbols provided by its stock host/elf_x86_64.x linker script. Derive a local
# linker script from the verified BlackBear source and add only those exact
# PROVIDE_HIDDEN contracts.
python3 - "$SOURCE" "$SCRIPT" <<'PY'
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text()
out = Path(sys.argv[2])

old_hdr = """    .eh_frame_hdr : ALIGN(CONSTANT(MAXPAGESIZE)) {
        KEEP(*(.eh_frame_hdr))
    } : ro
    .eh_frame : { KEEP(*(.eh_frame)) } : ro
"""
new_hdr = """    .eh_frame_hdr : ALIGN(CONSTANT(MAXPAGESIZE)) {
        PROVIDE_HIDDEN(__eh_frame_hdr_start = .);
        KEEP(*(.eh_frame_hdr))
        PROVIDE_HIDDEN(__eh_frame_hdr_end = .);
    } : ro
    .eh_frame : {
        PROVIDE_HIDDEN(__eh_frame_start = .);
        KEEP(*(.eh_frame))
        PROVIDE_HIDDEN(__eh_frame_end = .);
    } : ro
"""
if source.count(old_hdr) != 1:
    raise SystemExit("unexpected BlackBear EH-frame linker layout")
source = source.replace(old_hdr, new_hdr, 1)

old_bss = """    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } : data
"""
new_bss = """    .bss : {
        PROVIDE_HIDDEN(__bss_start = .);
        *(.bss .bss.*)
        *(COMMON)
        PROVIDE_HIDDEN(__bss_end = .);
    } : data
"""
if source.count(old_bss) != 1:
    raise SystemExit("unexpected BlackBear BSS linker layout")
source = source.replace(old_bss, new_bss, 1)

out.write_text(source)
PY

printf '%s\n' "$SCRIPT"
