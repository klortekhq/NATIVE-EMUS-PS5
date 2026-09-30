# RPCS3 / PlayStation 3

## Current situation

This target is moving rapidly in the PS5 scene.

Primary external references:

- mihawk-99/PS5_RPCS3
- mihawk-99/PS5_LLVM
- mihawk-99/PS5_Mesa

Recent work proves that PS5-specific issues are being found inside LLVM, SPU compilation and RADV command submission, not just in frontend glue.

## Desired architecture

```text
PPU/SPU guest
  -> RPCS3 LLVM/JIT
  -> PS5-specific LLVM ABI support
  -> Zen 2

RSX
  -> RPCS3 Vulkan
  -> PS5 Mesa/RADV
```

## Critical areas

- LLVM PS5 ABI/alignment behavior
- SPU compiler stability
- executable memory
- huge VM/address-space assumptions
- renderer submission cost
- threaded Vulkan recording
- synchronization/suspend-point behavior
- Qt/frontend removal
- audio/input replacement

## Rule

Do not duplicate active PS5_RPCS3 work blindly. Track, understand and integrate cleanly.

## Next milestone

Produce a reproducible map of the PS5_RPCS3 delta grouped into:

1. LLVM/toolchain
2. memory/JIT
3. Vulkan/RADV
4. platform services
5. frontend/headless
6. performance instrumentation
