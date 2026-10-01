# RPCS3 / PlayStation 3

## Progress: **30%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

## Current situation

This target is moving rapidly in the PS5 scene.

Current CPU bring-up source of truth:

- RPCS3 canonical: `RPCS3/rpcs3@83b1e072990e839903c67401d1a6d7d1910a5824`;
- RPCS3-pinned LLVM submodule: `llvm/llvm-project@ca7933e47d3a3451d81e72ac174dcb5aa28b59d1`;
- RPCS3-pinned AsmJit: `asmjit/asmjit@416f7356967c1f66784dc1580fe157f9406d8bff`.

The RPCS3 pin was re-audited on 2026-10-01 and is the current canonical
upstream head at the time of this gate. Historical `mihawk-99/PS5_RPCS3`,
`PS5_LLVM` and `PS5_Mesa` revisions are retained in the scene cache as
provenance/reference material, but are not required live dependencies.

The active PS5 CPU gate reproduces the SCE x86-64 over-alignment defect against
the canonical LLVM submodule, applies a minimal marker-checked shim only under
`__SCE__`, and cross-compiles the real PPU/SPU recompiler translation units.
No generated LLVM header is borrowed from another revision.

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

Do not resurrect unavailable forks as hidden dependencies. Start from canonical
RPCS3 and its own pinned third-party revisions, then reapply only PS5-specific
behavior that is independently reproduced and documented.

## Next milestone

Close the canonical PPU+SPU CPU archive gate, then continue in this order:

1. executable-memory/JIT runtime adaptation;
2. Vulkan/RADV baseline selection from current public PS5 references;
3. platform services;
4. frontend/headless isolation;
5. performance instrumentation.
