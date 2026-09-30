# FreeIntv

## Progress: **50%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

**Systems:** Intellivision  
**Repository state:** Port research / native PS5 integration plan

## Why this core is tracked

Historical discussed target; low platform complexity.

## Host architecture

Portable core.

The PS5 adaptation should use the shared project services whenever possible:

- `ps5rt::memory` / `ps5rt::jit` for executable and special mappings;
- `ps5rt::audio` for native AudioOut;
- `ps5rt::input` for DualSense and optional keyboard/mouse;
- `ps5rt::vfs` for internal, USB/M.2 and optional SMB content;
- `ps5rt::app` for lifecycle/logging;
- Vulkan/RADV only when this core actually benefits from a hardware renderer.

## Port rule

Keep the upstream emulator core recognizable. Do not make RetroArch/libretro a mandatory runtime layer merely because a libretro port already exists; use it as a donor/reference for PS5-specific fixes where useful.

## First PS5 milestone

1. pin a reproducible upstream revision;
2. compile the core with the PS5 toolchain;
3. boot a minimal test ROM/content path;
4. validate audio/input/save paths;
5. only then add a full native frontend and packaging.
