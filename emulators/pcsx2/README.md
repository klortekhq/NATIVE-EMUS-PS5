# PCSX2 / PlayStation 2

## Progress: **25%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

## Strategy

Use PCSX2 directly with its x86-64 recompilers and Vulkan renderer.

## Primary external reference

Swordpdf/PS5SX2 is currently the strongest public proof of this architecture on PS5.

Useful lessons already visible there:

- `-march=znver2`
- AVX2/SSE4.1 host tuning
- executable JIT shared memory
- flexible-memory mmap replacement
- explicit PS5 thread-stack handling
- AudioOut backend
- Vulkan command-stream and readback diagnostics
- native frontend separated from the core

## Project position

Do not rebuild PS2 emulation from Sony PS2 Classics components.

Do not require RetroArch.

The clean target is:

```text
PCSX2 core -> x86-64 recompilers -> PS5 CPU
PCSX2 Vulkan -> PS5 RADV/Vulkan -> PS5 GPU
```

## Next work

- diff PS5SX2 against its pinned PCSX2 upstream
- classify changes into runtime-common vs PCSX2-specific
- document licensing boundaries
- extract generic JIT/memory/thread/audio patterns into ps5rt
- keep PCSX2 upstream delta small
