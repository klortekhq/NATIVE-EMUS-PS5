# RPCS3 PS5 JIT / virtual-memory contract

Reviewed: 2026-10-01

This note records the **canonical upstream RPCS3** memory contract before any
PS5-specific allocator transformation is attempted.

Pinned CPU stack:

- RPCS3: `83b1e072990e839903c67401d1a6d7d1910a5824`
- LLVM submodule: `ca7933e47d3a3451d81e72ac174dcb5aa28b59d1`
- AsmJit submodule: `416f7356967c1f66784dc1580fe157f9406d8bff`

The historical Mihawk PS5 forks remain provenance references only. This document
describes what the **current canonical source** actually requires.

## AsmJit arena

`Utilities/JITASM.cpp` reserves one contiguous **2 GiB** host virtual range:

~~~text
0x00000000 .. 0x3fffffff   1 GiB executable/code
0x40000000 .. 0x7fffffff   1 GiB writable/data
~~~

Important details:

- total virtual reservation: `0x80000000` (2 GiB);
- each subrange is capped at `0x40000000` (1 GiB);
- physical backing is grown on demand;
- growth is rounded to **2 MiB** boundaries;
- generated code is requested executable;
- data allocations are writable;
- the address-space reservation is much larger than the amount normally
  committed at one time.

This distinction is critical on PS5: reserving 2 GiB of **virtual address
space** is acceptable engineering in principle; eagerly consuming 2 GiB of
flexible/direct physical memory is not an equivalent implementation.

## LLVM RTDyld arena

`Utilities/JITLLVM.cpp::MemoryManager1` uses:

- `c_max_size = 0x10000000` = **256 MiB** per logical region;
- one reservation of `c_max_size * 3` = **768 MiB**;
- code starts in the first 256 MiB;
- RW data currently starts in the second 256 MiB;
- the separate RO-data block is presently disabled in upstream;
- normal commit granularity is **2 MiB**;
- the first small allocations may commit **512 KiB** quarter-pages;
- LLVM code sections request executable memory;
- LLVM data sections request RW memory.

`MemoryManager2` falls back to the common `jit_runtime::alloc()` path.

## What ps5rt already provides

The current PS5 runtime already has the primitives needed for a large part of
this design:

- `ps5rt_vrange_reserve()` / `ps5rt_vrange_reserve_fixed()` for large
  virtual-address reservations;
- `ps5rt_vrange_release()`;
- `ps5rt_vmem_commit()`, `ps5rt_vmem_decommit()` and protection changes;
- direct-memory shared backing and fixed aliases through `ps5rt_shm_*`;
- executable shared memory;
- separate RW/RX JIT aliases through `ps5rt::JitRegion`;
- fixed-address controls for recompilers whose generated-code ABI needs them;
- instruction-cache flush.

## Gap before RPCS3 runtime integration

Do **not** replace RPCS3's 2 GiB reservation with one 2 GiB
`create_jit_region()` call.

The remaining PS5-specific gate is a sparse/reserved-arena policy that preserves
RPCS3's address-space semantics while committing backing incrementally:

1. reserve the large arena without eagerly backing all of it;
2. commit code/data chunks at the requested fixed offsets;
3. keep executable code out of the flexible-memory budget where practical;
4. preserve the 2 MiB / 512 KiB growth behavior or prove a safe PS5-specific
   replacement;
5. support protection/decommit semantics used by the canonical allocator;
6. validate the largest reservation and representative commits on physical PS5;
7. only then transform RPCS3's `utils::memory_*` / JIT allocator boundary.

The existing `ps5rt_vmem_commit()` currently maps **flexible memory** into a
reserved range. That is useful for small sparse regions but is not yet accepted
as the final RPCS3 code-cache policy because RPCS3's potential JIT footprint is
large and other PS5 ports have already shown why executable caches should avoid
exhausting flexible memory.

Likewise, creating one `ps5rt_shm` object for the whole 2 GiB arena would
reserve physical direct-memory backing up front and therefore would not preserve
upstream's sparse-commit behavior.

## Required proof

The next memory gate should be a dedicated native probe, not a full RPCS3 boot:

- reserve 2 GiB virtual space;
- establish code/data subranges;
- incrementally back representative 512 KiB and 2 MiB chunks;
- write x86-64 code through a writable mapping;
- execute it through an executable mapping;
- decommit/recommit a chunk without moving the reservation;
- verify neighbouring uncommitted ranges remain untouched;
- report direct/flexible memory consumption before and after.

Only after that probe is green should the RPCS3 allocator be routed through the
shared runtime.
