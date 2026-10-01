#!/usr/bin/env python3
"""Verify the canonical RPCS3 CPU-engine source boundary for native PS5 work."""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess

RPCS3_PIN = "83b1e072990e839903c67401d1a6d7d1910a5824"
LLVM_PIN = "ca7933e47d3a3451d81e72ac174dcb5aa28b59d1"
ASMJIT_PIN = "416f7356967c1f66784dc1580fe157f9406d8bff"


def head(path: pathlib.Path) -> str:
    return subprocess.check_output(
        ["git", "-C", str(path), "rev-parse", "HEAD"], text=True
    ).strip()


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"{label}: missing {needle!r}")


def require_re(text: str, pattern: str, label: str) -> None:
    if not re.search(pattern, text, flags=re.MULTILINE):
        raise SystemExit(f"{label}: missing /{pattern}/")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rpcs3", type=pathlib.Path)
    ap.add_argument("llvm", type=pathlib.Path)
    a = ap.parse_args()

    rpcs3 = a.rpcs3.resolve()
    llvm = a.llvm.resolve()
    asmjit = rpcs3 / "3rdparty/asmjit/asmjit"

    if head(rpcs3) != RPCS3_PIN:
        raise SystemExit(f"RPCS3 pin mismatch; expected {RPCS3_PIN}")
    if head(llvm) != LLVM_PIN:
        raise SystemExit(f"LLVM pin mismatch; expected {LLVM_PIN}")
    if head(asmjit) != ASMJIT_PIN:
        raise SystemExit(f"AsmJit pin mismatch; expected {ASMJIT_PIN}")

    emu = (rpcs3 / "rpcs3/Emu/CMakeLists.txt").read_text()
    require(emu, "Cell/PPUTranslator.cpp", "PPU LLVM translator")
    require(emu, "Cell/SPULLVMRecompiler.cpp", "SPU LLVM recompiler")
    require(emu, "Cell/SPUASMJITRecompiler.cpp", "SPU ASMJIT support")
    require_re(
        emu,
        r"add_library\s*\(\s*rpcs3_emu\s+STATIC",
        "standalone engine boundary",
    )

    llvm_cmake = (rpcs3 / "3rdparty/llvm/CMakeLists.txt").read_text()
    require(
        llvm_cmake,
        "find_package(LLVM 22.1 CONFIG)",
        "RPCS3 pinned LLVM contract",
    )

    # The historical PS5 LLVM fork carried SCE-specific alignment fixes here.
    # Canonical LLVM now expresses those requirements generically. Prove that
    # the exact pinned source still has those invariants before cross-building.
    small = (llvm / "llvm/include/llvm/ADT/SmallVector.h").read_text()
    require(
        small,
        "struct SmallVectorAlignmentAndSize",
        "SmallVector alignment helper",
    )
    require(
        small,
        "alignas(T) char FirstEl",
        "SmallVector element alignment",
    )

    trailing = (llvm / "llvm/include/llvm/Support/TrailingObjects.h").read_text()
    require(
        trailing,
        "MaxAlignment<TrailingTys...>",
        "TrailingObjects maximum alignment",
    )
    require_re(
        trailing,
        r"class\s+alignas\(Align\)\s+TrailingObjectsImpl",
        "TrailingObjects aligned implementation",
    )

    cpu_translator = (rpcs3 / "rpcs3/Emu/CPU/CPUTranslator.h").read_text()
    require(
        cpu_translator,
        "LLVM_VERSION_MAJOR >= 23",
        "forward LLVM API compatibility",
    )

    print("RPCS3 canonical PS5 CPU source gate PASS")
    print(f"rpcs3={RPCS3_PIN}")
    print(f"llvm={LLVM_PIN}")
    print(f"asmjit={ASMJIT_PIN}")
    print("cpu=PPU LLVM + SPU LLVM/ASMJIT")
    print("frontend_boundary=rpcs3_emu static engine")
    print("llvm_alignment=canonical generic alignment contracts")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
