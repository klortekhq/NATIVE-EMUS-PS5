#!/usr/bin/env python3
"""Retarget Mihawk's PS5 Dynarmic allocator to the shared ps5rt ABI."""

from __future__ import annotations
import argparse
import pathlib
import subprocess

EXPECTED = "1df2a07e9a86afef73d511a931f40bedb40b30c4"
TARGET = pathlib.Path("src/dynarmic/backend/x64/block_of_code.cpp")

def transform(text: str) -> str:
    replacements = (
        ("#    include <ps5platform/exec.h>", "#    include <ps5rt/c/exec.h>"),
        ("ps5_exec_allocate(size, reinterpret_cast<uintptr_t>(this))",
         "ps5rt_exec_allocate(size, reinterpret_cast<uintptr_t>(this))"),
        ("ps5_exec_release(p)", "ps5rt_exec_release(p)"),
    )
    for old, new in replacements:
        count = text.count(old)
        if count != 1:
            raise RuntimeError(f"expected one occurrence of {old!r}, found {count}")
        text = text.replace(old, new, 1)
    return text

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=pathlib.Path)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    root = args.source.resolve()

    head = subprocess.check_output(
        ["git", "-C", str(root), "rev-parse", "HEAD"], text=True
    ).strip()
    if head != EXPECTED:
        raise SystemExit(f"wrong PS5_Dynarmic revision: {head}; expected {EXPECTED}")
    if subprocess.run(["git", "-C", str(root), "diff", "--quiet"]).returncode:
        raise SystemExit("Dynarmic checkout has local modifications")

    path = root / TARGET
    try:
        updated = transform(path.read_text())
    except RuntimeError as exc:
        raise SystemExit(str(exc))

    if args.check:
        print("Dynarmic PS5 allocator -> ps5rt transform matched")
        return 0

    path.write_text(updated)
    (root / ".native-emus-ps5-ps5rt").write_text(EXPECTED + "\n")
    print("Dynarmic PS5 allocator retargeted to ps5rt")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
