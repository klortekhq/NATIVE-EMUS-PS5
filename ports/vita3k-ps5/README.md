# Vita3K Dynarmic PS5 engine gate

This port gate builds the **exact Dynarmic fork/revision pinned by Vita3K** as
an A32 -> x86-64 JIT for PlayStation 5.

Pinned source:

- repository: `Vita3K/dynarmic`
- commit: `86458a0bd369d63ba4c2ef812cacbb6c9080c065`

## Why this exists

Vita3K does not consume the same Dynarmic revision as every other emulator.
Replacing its submodule with another fork would risk API and behavioral drift.

Instead, this gate changes only the host executable-memory allocator:

```text
Vita ARMv7
   -> Vita3K Dynarmic A32
   -> x86-64 codegen
   -> ps5rt_exec_allocate/release
   -> PS5 Zen 2
```

No interpreter backend is substituted.

## Success condition

The PS5 cross-build must emit `libvita3k_dynarmic_ps5.a` containing at least:

- `block_of_code.cpp.o`
- `a32_jitstate.cpp.o`

and the transformed allocator must call `ps5rt_exec_allocate`.
