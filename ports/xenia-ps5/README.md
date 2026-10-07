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

## Fixed-address requirement

Xenia's x64 backend uses fixed low host ranges:

- indirection table: `0x80000000`
- execute view: `0xA0000000`
- write alias: `0xB0000000`
- generated-code backing: 256 MiB

`ps5rt::JitRequest` therefore supports fixed write/execute aliases backed by one direct-memory allocation.

## Current transform

`tools/ps5/apply_xenia_ps5rt.py` retargets the generated-code cache only:

- preserves X64Backend/Xbyak/HIR;
- owns a `ps5rt::JitRegion`;
- maps RX at `0xA0000000`;
- maps RW at `0xB0000000`;
- maps the entire backing up front.

## Next blocker

The separate fixed indirection table at `0x80000000` still needs a ps5rt mapping before a complete CPU-engine cross-build is claimed.
