# Mupen64Plus / Nintendo 64

## Progress: **45%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

## Status

**Dual-JIT PS5 allocator integration active.**

The native target keeps both compilation engines enabled:

```text
MIPS III guest CPU
  -> Mupen64Plus x86-64 new dynarec
  -> ps5rt executable memory
  -> Zen 2

RSP microcode
  -> ParaLLEl-RSP JIT
  -> ps5rt executable memory
  -> Zen 2
```

## Why two JIT allocators matter

The PS5 donor found that ordinary/flexible executable memory is the wrong place
for these caches:

- CPU dynarec cache: ~32 MiB
- ParaLLEl-RSP blocks: 64 MiB each

The donor therefore uses console executable/direct memory. Our deterministic
transform retargets both allocations to `ps5rt_exec_allocate/release`, so N64
shares the same executable-memory backend as PPSSPP, Dynarmic and other JIT cores.

## Renderer

Preferred path:

```text
RDP -> ParaLLEl-RDP -> Vulkan -> PS5 Mesa/RADV
```

GLideN64/OpenGL is not the target renderer for the PS5 native build.

## Current gates

- [x] PS5 CPU dynarec allocator identified
- [x] PS5 ParaLLEl-RSP JIT allocator identified
- [x] both retargeted to shared ps5rt ABI
- [x] exact donor revision pinned
- [ ] prospero-clang static engine build
- [ ] ParaLLEl-RDP Vulkan integration
- [ ] physical ROM boot from standalone native title
