# RPCS3 PS5 — LLVM recompiler engine gate

Pinned canonical CPU stack:

- RPCS3: `RPCS3/rpcs3@83b1e072990e839903c67401d1a6d7d1910a5824`
- LLVM submodule: `llvm/llvm-project@ca7933e47d3a3451d81e72ac174dcb5aa28b59d1`
- AsmJit submodule: `asmjit/asmjit@416f7356967c1f66784dc1580fe157f9406d8bff`

Historical Mihawk PS5 pins remain recorded in `upstreams/scene-cache.json`
for provenance, but the CPU gate no longer depends on those unavailable repos.

## CPU rule

The PS5 target is:

```text
Cell PPU
  -> RPCS3 PPU LLVM translator
  -> PS5 LLVM
  -> x86-64 Zen 2

Cell SPU
  -> RPCS3 SPU LLVM recompiler
  -> PS5 LLVM
  -> x86-64 Zen 2
```

The interpreter is not the intended final backend.

## Current gate

`tools/ps5/verify_rpcs3_ps5_sources.py` verifies the exact RPCS3/LLVM pins, the PPU and SPU recompiler sources, the static `rpcs3_emu` engine boundary, and the two PS5 LLVM ABI alignment fixes.

Current gate: cross-compile the real PPU LLVM translator and SPU LLVM/AsmJit
recompilers from the canonical stack with the public Prospero compiler. The
workflow generates every required LLVM TableGen header from the exact same LLVM
revision and applies the independently reproduced SCE alignment shim.

After the CPU object/archive gate is green, connect executable-memory runtime
services and then Vulkan/RADV.


## Verified PS5 compiler gates

The following gates have already passed with the public PS5 toolchain:

- PS5_LLVM ABI alignment probe for `SmallVector<T,0>` and `TrailingObjects`;
- real `Utilities/JITASM.cpp` compilation;
- real `SPUASMJITRecompiler.cpp` compilation.

The next LLVM gate compiles `PPUTranslator.cpp` and
`SPULLVMRecompiler.cpp`. LLVM source headers and generated TableGen IR
headers must come from the **same PS5_LLVM commit**. Mixing the newer source
tree with generated `.inc` files from another LLVM version is explicitly
forbidden by the workflow.
