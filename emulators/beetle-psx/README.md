# Beetle PSX / Beetle PSX HW

## Progress: **80%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

**Systems:** PlayStation  
**Repository state:** Native PS5 engine cross-built / standalone integration in progress

## Why this core is tracked

`libretro/beetle-psx-libretro@ed87921996c67658d7a70814f73034bbca08786a` is the canonical source. Mihawk's earlier PS5_BeetlePSX work remains historical feasibility evidence, but the native port no longer depends on that now-unavailable fork.

## Host architecture

Release CPU path: **Lightrec + GNU Lightning x86-64 recompilation on PS5 Zen 2**. Interpreter modes are diagnostic only. Hardware rendering uses Beetle's Vulkan RHI through the native PS5 Vulkan host.

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

## Current evidence

Workflow **36833195325** is green for the current public Beetle pin and validates the Lightrec x86-64 transform, native Vulkan host contracts, local multi-disc M3U handling, per-track optical sector widths, AudioOut/DualSense and standalone shell compilation. Full pinned RADV link and physical-console validation remain.
