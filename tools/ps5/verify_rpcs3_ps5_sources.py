#!/usr/bin/env python3
"""Verify the RPCS3 PS5 CPU-engine source boundary and PS5 LLVM ABI fixes."""

from __future__ import annotations
import argparse, pathlib, subprocess

RPCS3_PIN="2ada8e453c592d8764d2801a667f3da415794918"
LLVM_PIN="98b45cc22e98844b7d49bbedefdfee04a740650a"

def head(path:pathlib.Path)->str:
    return subprocess.check_output(["git","-C",str(path),"rev-parse","HEAD"],text=True).strip()

def require(text:str, needle:str, label:str)->None:
    if needle not in text:
        raise SystemExit(f"{label}: missing {needle!r}")

def main()->int:
    ap=argparse.ArgumentParser()
    ap.add_argument("rpcs3",type=pathlib.Path)
    ap.add_argument("llvm",type=pathlib.Path)
    a=ap.parse_args()
    rpcs3=a.rpcs3.resolve(); llvm=a.llvm.resolve()
    if head(rpcs3)!=RPCS3_PIN: raise SystemExit(f"RPCS3 pin mismatch; expected {RPCS3_PIN}")
    if head(llvm)!=LLVM_PIN: raise SystemExit(f"PS5 LLVM pin mismatch; expected {LLVM_PIN}")

    emu=(rpcs3/"rpcs3/Emu/CMakeLists.txt").read_text()
    require(emu,"Cell/PPUTranslator.cpp","PPU LLVM translator")
    require(emu,"Cell/SPULLVMRecompiler.cpp","SPU LLVM recompiler")
    require(emu,"Cell/SPUASMJITRecompiler.cpp","SPU ASM/JIT support")
    require(emu,"add_library(rpcs3_emu STATIC","RPCS3 engine target")

    top=(rpcs3/"rpcs3/CMakeLists.txt").read_text()
    require(top,"if(BUILD_LIBRETRO)","minimal frontend build boundary")
    require(top,"rpcs3_emu","engine link target")

    third=(rpcs3/"3rdparty/CMakeLists.txt").read_text()
    require(third,"if (BUILD_LIBRETRO)","desktop dependency exclusions")
    require(third,"add_subdirectory(volk EXCLUDE_FROM_ALL)","Vulkan loader boundary")

    small=(llvm/"llvm/include/llvm/ADT/SmallVector.h").read_text()
    require(small,"#if defined(__SCE__)","SmallVector PS4/PS5 ABI fix")
    require(small,"LLVM_SMALLVECTOR_ALIGNAS","SmallVector explicit alignment")

    trailing=(llvm/"llvm/include/llvm/Support/TrailingObjects.h").read_text()
    require(trailing,"#if defined(__SCE__)","TrailingObjects PS4/PS5 ABI fix")
    require(trailing,"AlignTrailingObjects[0]","TrailingObjects aligned base workaround")

    print("RPCS3 PS5 CPU engine source gate PASS")
    print(f"rpcs3={RPCS3_PIN}")
    print(f"llvm={LLVM_PIN}")
    print("cpu=PPU LLVM + SPU LLVM/ASMJIT")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
