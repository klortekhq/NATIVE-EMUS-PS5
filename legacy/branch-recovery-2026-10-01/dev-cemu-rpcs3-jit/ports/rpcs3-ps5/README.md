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

## Why the LLVM pin is newer than RPCS3's ordinary submodule pin

The RPCS3 tree currently references LLVM `ca7933e...`. Mihawk's PS5 LLVM
then added two PS4/PS5 ABI fixes that matter directly to RPCS3:

1. `f8d662c...`: preserve `SmallVector<T,0>` element alignment on the
   SCE x86-64 ABI.
2. `98b45cc...`: fix `TrailingObjects` alignment. The commit specifically
   documents an RPCS3 SPU compiler crash in
   `MachineInstr::hasOrderedMemoryRef` on PS5.

For that reason, our PS5 CPU gate refuses the older LLVM source and requires
the latter commit.

## Build boundary

Mihawk's RPCS3 fork already has a `BUILD_LIBRETRO` configuration that removes
large portions of the desktop Qt/OpenGL/cubeb/HID shell while retaining
`rpcs3_emu`, Vulkan and the Cell recompilers.

We may use that option as a **build boundary** while developing the engine.
It does not make RetroArch part of the final NATIVE-EMUS-PS5 architecture.

## Current gate

`tools/ps5/verify_rpcs3_ps5_sources.py` verifies:

- exact RPCS3 and LLVM revisions;
- `rpcs3_emu` is a static engine target;
- PPU LLVM translator is present;
- SPU LLVM and ASM/JIT sources are present;
- minimal-build dependency exclusions exist;
- both critical SCE LLVM ABI fixes exist.

The next gate is a PS5 cross-build of `rpcs3_emu` against this exact LLVM
fork, followed by Vulkan/RADV integration.
