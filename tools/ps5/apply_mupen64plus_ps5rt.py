#!/usr/bin/env python3
"""Retarget Mihawk's PS5 Mupen64Plus CPU/RSP JIT allocators to ps5rt."""

from __future__ import annotations
import argparse
import pathlib
import subprocess

EXPECTED = "98ec1019d191fc3c0e1c2d031f2574e95bd8d30d"
FILES = (
    pathlib.Path("mupen64plus-core/src/device/r4300/new_dynarec/new_dynarec.c"),
    pathlib.Path("mupen64plus-rsp-paraLLEl/jit_allocator.cpp"),
)

def transform(text: str) -> str:
    replacements = (
        ("<ps5platform/exec.h>", "<ps5rt/c/exec.h>"),
        ("ps5_exec_allocate", "ps5rt_exec_allocate"),
        ("ps5_exec_release", "ps5rt_exec_release"),
    )
    for old, new in replacements:
        if old in text:
            text = text.replace(old, new)
    # Comments in the donor intentionally mention ps5platform/exec.h when
    # explaining the original implementation. Validate code-bearing tokens,
    # not prose, so documentation does not become a false positive.
    stale = (
        "#include <ps5platform/exec.h>" in text
        or "ps5_exec_allocate(" in text
        or "ps5_exec_release(" in text
    )
    if stale:
        raise RuntimeError("PS5 allocator donor symbols remain after transform")
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
        raise SystemExit(f"wrong PS5_Mupen64Plus revision: {head}; expected {EXPECTED}")
    if subprocess.run(["git", "-C", str(root), "diff", "--quiet"]).returncode:
        raise SystemExit("Mupen64Plus donor checkout has local modifications")

    changed = 0
    outputs = {}
    for rel in FILES:
        path = root / rel
        before = path.read_text()
        after = transform(before)
        if before == after:
            raise SystemExit(f"{rel}: expected PS5 allocator symbols were not found")
        outputs[path] = after
        changed += 1

    if args.check:
        print(f"Mupen64Plus {EXPECTED}: {changed} JIT allocator files retarget cleanly")
        return 0

    for path, text in outputs.items():
        path.write_text(text)
    (root / ".native-emus-ps5-ps5rt").write_text(EXPECTED + "\n")
    print("Mupen64Plus CPU + ParaLLEl-RSP JIT allocators retargeted to ps5rt")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
