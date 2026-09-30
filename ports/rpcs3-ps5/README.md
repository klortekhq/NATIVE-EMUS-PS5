# RPCS3 PS5 — LLVM recompiler engine gate

Pinned donor sources:

- RPCS3: `mihawk-99/PS5_RPCS3@2ada8e453c592d8764d2801a667f3da415794918`
- LLVM: `mihawk-99/PS5_LLVM@98b45cc22e98844b7d49bbedefdfee04a740650a`

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

Next gate: cross-build `rpcs3_emu` with the pinned PS5 LLVM fork, then connect Vulkan/RADV.
