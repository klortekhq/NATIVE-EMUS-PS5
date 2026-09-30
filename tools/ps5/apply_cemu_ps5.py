#!/usr/bin/env python3
"""Deterministically adapt pinned Cemu x64 recompiler sources for PS5 headers."""

from __future__ import annotations
import argparse
import pathlib
import subprocess

PIN = "4e3c824faa00f6b85782db019f20f29f063f3a2a"

def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected 1 match, found {count}")
    return text.replace(old, new, 1)

def transform(root: pathlib.Path) -> dict[pathlib.Path, str]:
    out: dict[pathlib.Path, str] = {}

    platform = root / "src/Common/platform.h"
    text = platform.read_text()
    text = replace_once(
        text,
        "#if BOOST_OS_WINDOWS\n",
        """#if defined(__SCE__)
#include <cerrno>
#ifndef bswap_16
#define bswap_16(v) __builtin_bswap16((uint16_t)(v))
#endif
#ifndef bswap_32
#define bswap_32(v) __builtin_bswap32((uint32_t)(v))
#endif
#ifndef bswap_64
#define bswap_64(v) __builtin_bswap64((uint64_t)(v))
#endif
#include "Common/unix/platform.h"
#elif BOOST_OS_WINDOWS
""",
        "PS5 Common platform branch",
    )
    out[platform] = text

    mmu = root / "src/Cafe/HW/MMU/MMU.h"
    text = mmu.read_text()
    text = replace_once(
        text,
        "#if BOOST_OS_WINDOWS\n#define CPU_swapEndianU64(_v) _byteswap_uint64((uint64)(_v))\n",
        """#if defined(__SCE__)
#define CPU_swapEndianU64(_v) __builtin_bswap64((uint64)(_v))
#define CPU_swapEndianU32(_v) __builtin_bswap32((uint32)(_v))
#define CPU_swapEndianU16(_v) __builtin_bswap16((uint16)(_v))
#elif BOOST_OS_WINDOWS
#define CPU_swapEndianU64(_v) _byteswap_uint64((uint64)(_v))
""",
        "PS5 MMU endian branch",
    )
    out[mmu] = text

    return out

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=pathlib.Path)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    root = args.source.resolve()
    head = subprocess.check_output(
        ["git", "-C", str(root), "rev-parse", "HEAD"], text=True
    ).strip()
    if head != PIN:
        raise SystemExit(f"wrong Cemu revision: {head}; expected {PIN}")
    if subprocess.run(["git", "-C", str(root), "diff", "--quiet"]).returncode:
        raise SystemExit("Cemu checkout has local modifications")

    try:
        changes = transform(root)
    except RuntimeError as exc:
        raise SystemExit(str(exc))

    if args.check:
        print(f"Cemu {PIN}: {len(changes)} PS5 platform transformations matched")
        return 0

    for path, content in changes.items():
        path.write_text(content)
    (root / ".native-emus-ps5").write_text(PIN + "\n")
    print("Cemu PS5 platform transformation applied")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
