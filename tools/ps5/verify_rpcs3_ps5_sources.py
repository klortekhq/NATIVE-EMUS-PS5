#!/usr/bin/env python3
"""Verify the RPCS3 PS5 CPU-engine source boundary and PS5 LLVM ABI fixes."""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess

RPCS3_PIN = "2ada8e453c592d8764d2801a667f3da415794918"
LLVM_PIN = "98b45cc22e98844b7d49bbedefdfee04a740650a"


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

    if head(rpcs3) != RPCS3_PIN:
        raise SystemExit(f"RPCS3 pin mismatch; expected {RPCS3_PIN}")
    if head(llvm) != LLVM_PIN:
        raise SystemExit(f"PS5 LLVM pin mismatch; expected {LLVM_PIN}")

    emu = (rpcs3 / "rpcs3/Emu/CMakeLists.txt").read_text()
    require(emu, "Cell/PPUTranslator.cpp", "PPU LLVM translator")
    require(emu, "Cell/SPULLVMRecompiler.cpp", "SPU LLVM recompiler")
    require(emu, "Cell/SPUASMJITRecompiler.cpp", "SPU ASM/JIT support")
    require_re(emu, r"add_library\s*\(\s*rpcs3_emu\s+STATIC", "RPCS3 engine target")

    # The PS5 donor currently uses BUILD_LIBRETRO as a minimal non-Qt engine
    # boundary. This repository does not require RetroArch itself: the useful
    # property here is that rpcs3_emu can be built without the desktop Qt shell.
    top = (rpcs3 / "rpcs3/CMakeLists.txt").read_text()
    require_re(top, r"if\s*\(\s*BUILD_LIBRETRO\s*\)", "minimal frontend build boundary")
    require(top, "rpcs3_emu", "engine link target")
    require_re(
        top,
        r"if\s*\(\s*NOT\s+ANDROID\s+AND\s+NOT\s+BUILD_LIBRETRO\s*\)",
        "Qt exclusion boundary",
    )

    third = (rpcs3 / "3rdparty/CMakeLists.txt").read_text()
    require_re(
        third,
        r"if\s*\(\s*BUILD_LIBRETRO\s*\)",
        "desktop dependency exclusions",
    )
    require(third, "3rdparty_volk", "Vulkan loader boundary")
    require(third, "VK_NO_PROTOTYPES", "frontend-provided Vulkan dispatch boundary")

    small = (llvm / "llvm/include/llvm/ADT/SmallVector.h").read_text()
    require(small, "#if defined(__SCE__)", "SmallVector PS4/PS5 ABI fix")
    require(small, "LLVM_SMALLVECTOR_ALIGNAS", "SmallVector explicit alignment")
    require(small, "alignas(void *)", "SmallVector pointer alignment floor")

    trailing = (llvm / "llvm/include/llvm/Support/TrailingObjects.h").read_text()
    require(trailing, "#if defined(__SCE__)", "TrailingObjects PS4/PS5 ABI fix")
    require(trailing, "AlignTrailingObjects[0]", "TrailingObjects aligned base workaround")

    print("RPCS3 PS5 CPU engine source gate PASS")
    print(f"rpcs3={RPCS3_PIN}")
    print(f"llvm={LLVM_PIN}")
    print("cpu=PPU LLVM + SPU LLVM/ASMJIT")
    print("frontend_boundary=non-Qt BUILD_LIBRETRO engine boundary")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
