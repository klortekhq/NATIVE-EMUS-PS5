#!/usr/bin/env python3
"""Retarget Vita3K's pinned Dynarmic x64 code allocator to ps5rt."""

from __future__ import annotations

import argparse
import pathlib
import subprocess

EXPECTED = "86458a0bd369d63ba4c2ef812cacbb6c9080c065"
TARGET = pathlib.Path("src/dynarmic/backend/x64/block_of_code.cpp")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one exact match, found {count}")
    return text.replace(old, new, 1)


def transform(text: str) -> str:
    text = replace_once(
        text,
        """#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#else
#    include <sys/mman.h>
#endif
""",
        """#ifdef __PROSPERO__
#    include <ps5rt/c/exec.h>
#elif defined(_WIN32)
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#else
#    include <sys/mman.h>
#endif
""",
        "platform includes",
    )

    text = replace_once(
        text,
        """class CustomXbyakAllocator : public Xbyak::Allocator {
public:
#ifdef _WIN32
""",
        """class CustomXbyakAllocator : public Xbyak::Allocator {
public:
#ifdef __PROSPERO__
    uint8_t* alloc(size_t size) override {
        void* p = ps5rt_exec_allocate(size, 0);
        if (p == nullptr) {
            throw Xbyak::Error(Xbyak::ERR_CANT_ALLOC);
        }
        return static_cast<uint8_t*>(p);
    }

    void free(uint8_t* p) override {
        ps5rt_exec_release(p);
    }

    bool useProtect() const override { return false; }
#elif defined(_WIN32)
""",
        "PS5 allocator",
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
        raise SystemExit(f"wrong Vita3K Dynarmic revision: {head}; expected {EXPECTED}")

    if subprocess.run(["git", "-C", str(root), "diff", "--quiet"]).returncode:
        raise SystemExit("Vita3K Dynarmic checkout has local modifications")

    path = root / TARGET
    try:
        updated = transform(path.read_text())
    except RuntimeError as exc:
        raise SystemExit(str(exc))

    if args.check:
        print(f"Vita3K Dynarmic {EXPECTED}: PS5 allocator transform matched")
        return 0

    path.write_text(updated)
    (root / ".native-emus-ps5-vita3k-dynarmic").write_text(EXPECTED + "\n")
    print("Vita3K Dynarmic x64 allocator retargeted to ps5rt")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
