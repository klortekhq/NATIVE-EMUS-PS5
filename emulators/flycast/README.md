# Flycast / Dreamcast · Naomi · Atomiswave

## Progress: **35%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

## Status

**Pinned source + deterministic PS5 rec-x64/VM transformation.**

Upstream is pinned to `e36e9df2dcc1487acdb1dc7725766f1f5ba029b5`.

## CPU

Final backend: **Flycast rec-x64 / Xbyak SH4 dynarec** on the PS5 Zen 2 CPU.

The PS5 transform enables `FEAT_NO_RWX_PAGES` and maps Flycast's existing dual-address JIT interface directly onto `ps5rt::JitRegion`:

```text
Xbyak emitter -> RW alias
                   |
                   +-- cc_rx_offset --> RX alias -> Zen 2 execution
```

No interpreter fallback is part of the final architecture.

## Memory

`tools/ps5/apply_flycast_native.py` replaces only the platform virtual-memory backend:

- SH4 fastmem reservation;
- mirrored RAM mappings;
- on-demand FPCB/data pages;
- JIT RW/RX aliases;
- JIT lifecycle.

The SH4 compiler, block manager, Xbyak emitter and guest timing remain upstream Flycast code.

## Graphics

Target renderer remains:

```text
PowerVR2 -> Flycast Vulkan -> PS5 Mesa/RADV
```

No OpenGL compatibility wrapper is the intended final renderer.

## Systems from the same engine

- Dreamcast
- Sega Naomi
- Sammy Atomiswave

They share the native engine but keep separate compatibility targets and test suites.

## Current gates

- [x] rec-x64/Xbyak path identified and pinned
- [x] Flycast dual RW/RX ABI matched to ps5rt
- [x] SH4 fastmem/mirror adapter written
- [x] deterministic PS5 source transform
- [ ] prospero-clang cross-compile of transformed engine
- [ ] SH4 fastmem initialization on physical PS5
- [ ] Vulkan device/context on PS5 RADV
- [ ] Dreamcast BIOS boot
- [ ] Naomi boot
- [ ] Atomiswave boot
