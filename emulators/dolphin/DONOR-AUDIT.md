# Dolphin PS5 donor audit

Primary donor: `mihawk-99/PS5_RetroArch`.

## Reproducible donor pin

The public PS5 Dolphin build used:

- source repository: `libretro/dolphin`
- source commit: `c6630001e05780b7c03e661a4a539b59ef716ebc`
- donor patch: `patches/dolphin/ps5-port.patch`

The donor's hardware evidence records Wind Waker reaching its title screen on PS5 with:

- **CPU Core: JIT64**
- **Fastmem enabled**
- Vulkan through the PS5 Vulkan path
- clean shutdown in that capture

This is evidence for the host architecture, not a claim that our standalone title is already booting.

## What NATIVE-EMUS-PS5 retains

Our selective patch `patches/dolphin/c6630001-ps5-platform.patch` keeps only host/platform hunks needed by a standalone port:

| Area | Donor solution | Shared destination |
| --- | --- | --- |
| guest RAM | direct-memory shared object | `ps5rt_shm_*` |
| mirrored fastmem | fixed aliases over reserved VA | `ps5rt_shm_map` |
| fastmem arena | unbacked reservation | `ps5rt_vrange_*` |
| JIT caches | executable PS5 memory | `ps5rt_exec_*` |
| page faults | SIGSEGV + SIGBUS | Dolphin platform logic |
| huge lazy entry map | disabled on PS5 | Dolphin config |
| thread naming | skip unsafe FreeBSD path | Dolphin platform guard |
| desktop-only deps | disabled | PS5 build configuration |

## What is deliberately excluded

The following donor changes are useful for its RetroArch test harness but are not part of our clean standalone architecture:

- libretro frame pacing / duplicate-frame logic;
- FIFO debug/object-range instrumentation;
- frontend option files;
- shader debug modes;
- module/core loader runtime glue;
- RetroArch-specific input and presentation plumbing.

## Additional reference: mkwii-ps5 / WiiCompiled

The static-recompilation project is not a Dolphin replacement. It remains useful for PS5-native platform behavior:

- guest VA reservations and aliases;
- 16 KiB page granularity;
- cached direct memory;
- x86-64 context switching;
- native AudioOut/VideoOut/pad lifecycle.

## Licensing boundary

Dolphin source is GPL-2.0-or-later on the relevant files. The donor's separate
`ps5-libc-shims.cpp` is GPL-3.0-or-later and is **not copied into ps5rt**.
Missing libc pieces are handled only when actually required, with compatible
licensing and a clear boundary.
