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

`tools/ps5/verify_rpcs3_ps5_sources.py` verifies the exact RPCS3/LLVM/AsmJit
pins, the PPU and SPU recompiler sources, the static `rpcs3_emu` engine
boundary and the canonical LLVM baseline used by the SCE ABI probe.

The **CPU compile gate is green**. Workflow `36855520217` cross-compiled the
real `Utilities/JITASM.cpp`, `SPUASMJITRecompiler.cpp`,
`PPUTranslator.cpp` and `SPULLVMRecompiler.cpp`, then archived them as
`librpcs3_ps5_cpu_engines.a`.

- artifact: `native-rpcs3-ppu-spu-jit-ps5-engine`
- artifact ID: `11158467437`
- artifact ZIP SHA-256:
  `076d343e32b4c3e1f90636c38e326f9a20b72599236f8579d7b660603f35fd97`

The active gate is now executable-memory behavior. `ps5rt` has a sparse
fixed-address arena that reserves large VA ranges without eagerly backing them,
and `ps5rt-rpcs3-sparse-jit-probe` reproduces RPCS3's representative
512 KiB executable + 2 MiB writable commit pattern inside a 2 GiB reservation.

Workflow `36857108594` cross-built that native probe with Prospero. It is
contained in artifact `native-ps5rt-jit-probes` (ID `11159199014`, ZIP
SHA-256
`b63f1460514568529450bee49a9b24a7b0c4551d7a99ca8733bee5a2b95dc48e`).

This proves the implementation compiles for PS5; it does **not** yet prove the
2 GiB reservation/direct-memory fixed mapping on physical hardware. Physical
execution is the next gate. Only after that passes should RPCS3's canonical
`utils::memory_*` / JIT allocator boundary be transformed.


## Verified PS5 compiler gates

The following gates have passed with the public PS5 toolchain:

- canonical LLVM's SCE over-alignment defect reproduced;
- deterministic `__SCE__` alignment shim verified;
- matching IR, Analysis and ValueType TableGen headers generated from the same
  pinned LLVM tree;
- real `Utilities/JITASM.cpp` compilation;
- real `SPUASMJITRecompiler.cpp` compilation;
- real `PPUTranslator.cpp` compilation;
- real `SPULLVMRecompiler.cpp` compilation;
- static PPU/SPU CPU-engine archive uploaded;
- 2 GiB sparse-JIT probe cross-built and uploaded.

See `JIT-MEMORY.md` for why the large address-space reservation must remain
sparse instead of becoming a 2 GiB physical allocation.
