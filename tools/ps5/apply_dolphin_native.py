#!/usr/bin/env python3
"""Validate/apply the selective native PS5 Dolphin platform patch."""

from __future__ import annotations
import argparse
import pathlib
import subprocess

EXPECTED = "c6630001e05780b7c03e661a4a539b59ef716ebc"

def run(*args: str, cwd: pathlib.Path) -> None:
    subprocess.run(args, cwd=cwd, check=True)

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=pathlib.Path)
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--repo-root", type=pathlib.Path)
    args = ap.parse_args()

    source = args.source.resolve()
    root = (args.repo_root or pathlib.Path(__file__).resolve().parents[2]).resolve()
    patch = root / "patches/dolphin/c6630001-ps5-platform.patch"

    head = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True
    ).strip()
    if head != EXPECTED:
        raise SystemExit(f"wrong Dolphin revision: {head}; expected {EXPECTED}")
    if subprocess.run(["git", "-C", str(source), "diff", "--quiet"]).returncode:
        raise SystemExit("Dolphin checkout has local modifications")

    run("git", "apply", "--check", str(patch), cwd=source)
    if args.check:
        print(f"Dolphin {EXPECTED}: selective PS5 Jit64/fastmem patch applies")
        return 0

    run("git", "apply", str(patch), cwd=source)
    (source / ".native-emus-ps5-dolphin").write_text(EXPECTED + "\n")
    print(f"Dolphin {EXPECTED}: native PS5 platform patch applied")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
