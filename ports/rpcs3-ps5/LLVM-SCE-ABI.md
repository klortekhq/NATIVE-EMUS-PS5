# RPCS3 / LLVM SCE x86-64 alignment ABI

Reviewed: 2026-10-01

This document records a reproducible PS5 compiler ABI issue discovered while
moving the RPCS3 CPU gate away from the unavailable historical Mihawk forks and
back to canonical RPCS3 + canonical LLVM.

## Exact source baseline

- RPCS3: `83b1e072990e839903c67401d1a6d7d1910a5824`
- RPCS3 LLVM submodule:
  `ca7933e47d3a3451d81e72ac174dcb5aa28b59d1`
- RPCS3 AsmJit submodule:
  `416f7356967c1f66784dc1580fe157f9406d8bff`
- compiler: public `ps5-payload-dev` Prospero Clang toolchain
- target ABI macro: `__SCE__`

The canonical LLVM tree contains generic alignment machinery for both
`SmallVector` and `TrailingObjects`. Source inspection alone is **not**
sufficient proof that those layouts satisfy the SCE ABI.

## Reproduced failure

The RPCS3 gate compiles an over-aligned test payload:

~~~cpp
struct alignas(32) Payload {
  unsigned char bytes[32];
};
~~~

and checks:

~~~cpp
static_assert(
    alignof(llvm::SmallVector<Payload, 0>) >= alignof(Payload));
static_assert(alignof(Node) >= alignof(Payload));
~~~

where `Node` privately inherits
`llvm::TrailingObjects<Node, Payload>`.

On the exact canonical LLVM pin, compiled by `prospero-clang++`, the
unpatched probe fails with:

~~~text
alignof(llvm::SmallVector<Payload, 0>) : 8
alignof(Payload)                       : 32

alignof(Node)                          : 4
alignof(Payload)                       : 32
~~~

This is a real target-ABI result, not an inferred source-level concern.

## Historical evidence retained by this repository

The recovered RPCS3 research branch already documented two historical
PS5_LLVM fixes:

1. historical commit prefix `f8d662c...` — preserve
   `SmallVector<T, 0>` element alignment on the SCE x86-64 ABI;
2. historical PS5_LLVM tip
   `98b45cc22e98844b7d49bbedefdfee04a740650a` — fix
   `TrailingObjects` alignment; the recovered notes associate this with an
   RPCS3 SPU compiler crash in `MachineInstr::hasOrderedMemoryRef`.

The old repository is no longer publicly retrievable, so those commits are
preserved as provenance, **not copied as unverifiable source**.

## Reconstructed minimal shim

`tools/ps5/apply_llvm_sce_alignment.py` applies two narrowly scoped,
marker-checked transformations to the canonical LLVM source:

### SmallVector

For `__SCE__`, the final `SmallVector<T, N>` class receives explicit
`alignas(T)` through `LLVM_SMALLVECTOR_ALIGNAS(T)`.

Canonical LLVM already aligns `SmallVectorStorage<T, 0>`, but on SCE that
over-alignment is lost when it is represented only through the empty base.

### TrailingObjects

For `__SCE__`, the aligned terminal `TrailingObjectsImpl` base receives:

~~~cpp
alignas(Align) char AlignTrailingObjects[0];
~~~

The zero-length aligned member prevents the required alignment from existing
only in an empty base, which is the layout that the SCE ABI collapses.

## CI proof contract

`.github/workflows/rpcs3-gate.yml` deliberately performs the checks in this
order:

1. clone the exact canonical RPCS3/LLVM/AsmJit revisions;
2. generate the matching LLVM configuration/TableGen headers;
3. compile the ABI probe **without** the shim and require the known failure;
4. require the diagnostics to contain the measured `8 >= 32` and `4 >= 32`
   failures;
5. apply the deterministic SCE shim;
6. compile the **same** probe again and require success;
7. only then attempt RPCS3 JITASM, SPU ASMJIT, PPU LLVM and SPU LLVM objects.

If a future LLVM pin fixes the SCE ABI upstream, step 3 will unexpectedly pass
and the workflow will stop. That is intentional: it forces removal/review of
the local compatibility shim instead of carrying a stale patch forever.

## Rule for future updates

Never move this patch forward because the filenames still exist.

For every RPCS3/LLVM re-pin:

- compile the unpatched target ABI probe first;
- if it passes, remove the shim;
- if it fails with a different layout, re-audit the ABI before changing code;
- if it reproduces the same failure, reapply the minimal transform and prove
  the patched probe;
- keep the actual RPCS3 CPU-object compilation as the final evidence.

This keeps the port upstream-first while preserving the PS5-specific ABI
knowledge that disappeared with the historical donor repository.
