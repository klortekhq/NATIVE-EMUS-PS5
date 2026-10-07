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

    precompiled = root / "src/Common/precompiled.h"
    text = precompiled.read_text()
    text = replace_once(
        text,
        "#if defined(__GNUC__)\n#define ATTR_MS_ABI __attribute__((ms_abi))\n#else\n#define ATTR_MS_ABI\n#endif\n",
        """#if defined(__SCE__)
#define ATTR_MS_ABI
#elif defined(__GNUC__)
#define ATTR_MS_ABI __attribute__((ms_abi))
#else
#define ATTR_MS_ABI
#endif
""",
        "PS5 SysV host ABI attribute",
    )
    out[precompiled] = text

    allocator = root / "src/Cafe/HW/Espresso/Recompiler/IML/IMLRegisterAllocator.cpp"
    text = allocator.read_text()
    text = replace_once(
        text,
        "const IMLPhysReg intParamToPhysReg[3] = {IMLArchX86::PHYSREG_GPR_BASE + X86_REG_RCX, IMLArchX86::PHYSREG_GPR_BASE + X86_REG_RDX, IMLArchX86::PHYSREG_GPR_BASE + X86_REG_R8};",
        """#if defined(__SCE__)
        const IMLPhysReg intParamToPhysReg[3] = {IMLArchX86::PHYSREG_GPR_BASE + X86_REG_RDI, IMLArchX86::PHYSREG_GPR_BASE + X86_REG_RSI, IMLArchX86::PHYSREG_GPR_BASE + X86_REG_RDX};
#else
        const IMLPhysReg intParamToPhysReg[3] = {IMLArchX86::PHYSREG_GPR_BASE + X86_REG_RCX, IMLArchX86::PHYSREG_GPR_BASE + X86_REG_RDX, IMLArchX86::PHYSREG_GPR_BASE + X86_REG_R8};
#endif""",
        "PS5 SysV integer parameter registers",
    )
    text = replace_once(
        text,
        """\t\tvolatileRegs.SetAvailable(IMLArchX86::PHYSREG_GPR_BASE + X86_REG_RDX);
\t\tvolatileRegs.SetAvailable(IMLArchX86::PHYSREG_GPR_BASE + X86_REG_R8);""",
        """\t\tvolatileRegs.SetAvailable(IMLArchX86::PHYSREG_GPR_BASE + X86_REG_RDX);
#if defined(__SCE__)
\t\tvolatileRegs.SetAvailable(IMLArchX86::PHYSREG_GPR_BASE + X86_REG_RSI);
\t\tvolatileRegs.SetAvailable(IMLArchX86::PHYSREG_GPR_BASE + X86_REG_RDI);
#endif
\t\tvolatileRegs.SetAvailable(IMLArchX86::PHYSREG_GPR_BASE + X86_REG_R8);""",
        "PS5 SysV volatile GPR set",
    )
    text = replace_once(
        text,
        """\t\t// YMM0-YMM5 are volatile
\t\tfor (int i = 0; i <= 5; i++)
\t\t\tvolatileRegs.SetAvailable(IMLArchX86::PHYSREG_FPR_BASE + i);
\t\t// for YMM6-YMM15 only the upper 128 bits are volatile which we dont use""",
        """#if defined(__SCE__)
\t\t// SysV AMD64: all XMM/YMM registers are caller-saved.
\t\tfor (int i = 0; i <= 15; i++)
\t\t\tvolatileRegs.SetAvailable(IMLArchX86::PHYSREG_FPR_BASE + i);
#else
\t\t// Microsoft x64: XMM0-XMM5 are volatile.
\t\tfor (int i = 0; i <= 5; i++)
\t\t\tvolatileRegs.SetAvailable(IMLArchX86::PHYSREG_FPR_BASE + i);
\t\t// for YMM6-YMM15 only the upper 128 bits are volatile which we dont use
#endif""",
        "PS5 SysV volatile vector set",
    )
    out[allocator] = text

    backend = root / "src/Cafe/HW/Espresso/Recompiler/BackendX64/BackendX64.cpp"
    text = backend.read_text()
    text = replace_once(
        text,
        """\t\t// set parameters
\t\tx64Gen_mov_reg64_reg64(x64GenContext, X86_REG_RCX, REG_RESV_HCPU);
\t\tx64Gen_mov_reg64_imm64(x64GenContext, X86_REG_RDX, funcId);""",
        """\t\t// set host ABI parameters
#if defined(__SCE__)
\t\tx64Gen_mov_reg64_reg64(x64GenContext, X86_REG_RDI, REG_RESV_HCPU);
\t\tx64Gen_mov_reg64_imm64(x64GenContext, X86_REG_RSI, funcId);
#else
\t\tx64Gen_mov_reg64_reg64(x64GenContext, X86_REG_RCX, REG_RESV_HCPU);
\t\tx64Gen_mov_reg64_imm64(x64GenContext, X86_REG_RDX, funcId);
#endif""",
        "PS5 SysV HLE parameters",
    )
    text = replace_once(
        text,
        """\tx64Emit_mov_mem64_reg64(&x64GenContext, X86_REG_RDX, offsetof(PPCInterpreter_t, rspTemp), X86_REG_RSP);

\t// MOV RSP, RDX (ppc interpreter instance)
\tx64Gen_mov_reg64_reg64(&x64GenContext, REG_RESV_HCPU, X86_REG_RDX);""",
        """#if defined(__SCE__)
\t// SysV: argument 2 (PPCInterpreter_t*) is RSI.
\tx64Emit_mov_mem64_reg64(&x64GenContext, X86_REG_RSI, offsetof(PPCInterpreter_t, rspTemp), X86_REG_RSP);
\tx64Gen_mov_reg64_reg64(&x64GenContext, REG_RESV_HCPU, X86_REG_RSI);
#else
\tx64Emit_mov_mem64_reg64(&x64GenContext, X86_REG_RDX, offsetof(PPCInterpreter_t, rspTemp), X86_REG_RSP);

\t// MOV RSP, RDX (ppc interpreter instance)
\tx64Gen_mov_reg64_reg64(&x64GenContext, REG_RESV_HCPU, X86_REG_RDX);
#endif""",
        "PS5 SysV entry hCPU argument",
    )
    text = replace_once(
        text,
        """\t//JMP recFunc
\tx64Gen_jmp_reg64(&x64GenContext, X86_REG_RCX); // call argument 1""",
        """\t// JMP recFunc using host ABI argument 1.
#if defined(__SCE__)
\tx64Gen_jmp_reg64(&x64GenContext, X86_REG_RDI);
#else
\tx64Gen_jmp_reg64(&x64GenContext, X86_REG_RCX);
#endif""",
        "PS5 SysV entry code argument",
    )
    out[backend] = text

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
