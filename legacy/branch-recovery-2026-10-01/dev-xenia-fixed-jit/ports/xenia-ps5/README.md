# Xenia PS5 — x64 recompiler gate

Pinned upstream:

- `xenia-project/xenia@95a5c3ee250f80c3b9d139658649d9ffb6db3eec`

## CPU architecture

```text
Xbox 360 PowerPC
    -> Xenia PPC frontend / HIR
    -> X64Backend / Xbyak
    -> fixed dual-view ps5rt JIT cache
    -> PS5 Zen 2
```

The interpreter is not the intended PS5 backend.

## Why Xenia needs a stronger JIT contract

Xenia's x64 backend does not merely ask for arbitrary executable memory. Its
generated-code ABI uses fixed low virtual ranges:

- indirection table: `0x80000000`
- execute view: `0xA0000000`
- write alias: `0xB0000000`
- generated-code backing: 256 MiB

The execute and write ranges must refer to the same generated code.

For PS5, `ps5rt::JitRequest` now supports preferred/fixed write and execute
views. The fixed path uses one direct-memory backing allocation and maps it
twice: RX and RW.

## Current transform

`tools/ps5/apply_xenia_ps5rt.py` changes only the generated-code cache:

- owns a `ps5rt::JitRegion` under `__PROSPERO__`;
- maps RX at `0xA0000000`;
- maps RW at `0xB0000000`;
- preserves the upstream X64Backend/Xbyak/HIR pipeline;
- skips desktop commit calls because the PS5 backing is mapped up front.

## Next blocker

The separate fixed indirection table at `0x80000000` still uses Xenia's
generic host memory API. That must be mapped through ps5rt before a complete
CPU-engine cross-build is claimed.

The Vulkan backend already exists upstream and will be adapted to PS5 RADV
only after this CPU/address-space gate is stable.
