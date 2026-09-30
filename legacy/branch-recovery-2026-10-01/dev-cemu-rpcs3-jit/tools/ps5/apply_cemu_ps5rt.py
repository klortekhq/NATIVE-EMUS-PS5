#!/usr/bin/env python3
"""Retarget Cemu's x86-64 PPC recompiler code cache to ps5rt."""

from __future__ import annotations

import argparse
import pathlib
import subprocess

EXPECTED = "4e3c824faa00f6b85782db019f20f29f063f3a2a"
TARGET = pathlib.Path(
    "src/Cafe/HW/Espresso/Recompiler/BackendX64/BackendX64.cpp"
)


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one exact match, found {count}")
    return text.replace(old, new, 1)


def transform(text: str) -> str:
    text = replace_once(
        text,
        '#include "util/MemMapper/MemMapper.h"\n',
        '#include "util/MemMapper/MemMapper.h"\n'
        '#ifdef __PROSPERO__\n'
        '#include <ps5rt/c/exec.h>\n'
        '#endif\n',
        "PS5 executable-memory include",
    )

    text = replace_once(
        text,
        """		codeMemoryBlock = (uint8*)MemMapper::AllocateMemory(nullptr, codeMemoryBlockSize, MemMapper::PAGE_PERMISSION::P_RWX);
""",
        """#ifdef __PROSPERO__
		codeMemoryBlock = static_cast<uint8*>(
			ps5rt_exec_allocate(static_cast<size_t>(codeMemoryBlockSize), 0));
#else
		codeMemoryBlock = (uint8*)MemMapper::AllocateMemory(
			nullptr, codeMemoryBlockSize, MemMapper::PAGE_PERMISSION::P_RWX);
#endif
		if (codeMemoryBlock == nullptr)
			return nullptr;
""",
        "PPC x64 executable-memory allocation",
    )

    return text


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    root = args.source.resolve()
    head = subprocess.check_output(
        ["git", "-C", str(root), "rev-parse", "HEAD"], text=True
    ).strip()
    if head != EXPECTED:
        raise SystemExit(f"wrong Cemu revision: {head}; expected {EXPECTED}")

    if subprocess.run(["git", "-C", str(root), "diff", "--quiet"]).returncode:
        raise SystemExit("Cemu checkout has local modifications")

    path = root / TARGET
    try:
        updated = transform(path.read_text())
    except RuntimeError as exc:
        raise SystemExit(str(exc))

    if args.check:
        if "PPCRecompiler_generateX64Code" not in updated:
            raise SystemExit("x64 recompiler backend disappeared from pinned Cemu source")
        if "ps5rt_exec_allocate" not in updated:
            raise SystemExit("PS5 executable allocator was not injected")
        print(f"Cemu {EXPECTED}: x64 PPC recompiler -> ps5rt transform matched")
        return 0

    path.write_text(updated)
    (root / ".native-emus-ps5-cemu").write_text(EXPECTED + "\n")
    print("Cemu PPC x64 recompiler executable-memory path retargeted to ps5rt")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
