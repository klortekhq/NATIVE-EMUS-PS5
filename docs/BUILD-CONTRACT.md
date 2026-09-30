# Native Build Contract

This document defines how emulator targets are expected to ship.

## One system, one native target

The project may share PS5 host code, but the default deliverable remains **one independently runnable native target per emulated system**.

Examples:

```text
EMU-NES
EMU-SNES
EMU-N64
EMU-PS1
EMU-PS2
EMU-DREAMCAST
...
```

A multi-system upstream core may still be compiled more than once with a system-specific entry point/configuration when that produces a cleaner user experience.

## What may be shared

`runtime/` is the common PS5 host layer:

- memory/JIT allocation;
- TLS/thread policy;
- Vulkan/RADV bootstrap;
- AudioOut;
- DualSense / optional keyboard and mouse;
- VFS/storage/SMB;
- application lifecycle;
- logging/crash diagnostics;
- packaging helpers.

This shared layer must never become a second emulator frontend.

## Development output

During bring-up, a target may produce whichever native development artifact the active open-source toolchain requires, such as an ELF/FSELF-style development binary.

The development artifact is an implementation detail, not the product architecture.

## Release output

The preferred console-facing release form is a native PS5 application/package when the public homebrew toolchain supports it cleanly.

A release should contain:

- emulator executable;
- required redistributable runtime modules/libraries;
- `sce_sys` metadata/assets where applicable;
- license/third-party notices;
- default config;
- no BIOS, firmware, keys, ROMs, ISOs or proprietary Sony SDK content.

## No mandatory mega-frontend

A unified launcher may exist later as an optional convenience layer.

It must not be required for:

- booting a system emulator;
- loading content;
- saving configuration;
- updating the emulator core.

Each system target remains independently testable and maintainable.

## Upstream identity

Every build must record:

- emulator/core name;
- exact upstream revision;
- NATIVE-EMUS-PS5 patch revision;
- PS5 SDK/toolchain revision;
- Mesa/RADV revision when used;
- enabled CPU backend;
- renderer;
- build type.

## Native CPU rule

When upstream has a suitable x86-64 JIT/recompiler, use it directly on the PS5 host CPU rather than interposing another emulation layer.

Examples:

- PCSX2 x86-64 recompilers;
- Flycast x86-64 SH4 dynarec;
- Dolphin x86-64 JIT;
- Dynarmic x86-64 backend;
- RPCS3 LLVM PPU/SPU;
- Xenia x64 backend.

Interpreter modes remain valuable for bring-up/debugging, but they are not a substitute for the native high-performance backend when one exists.

## Build reproducibility

A target cannot be marked `Validated` until its build can be reproduced from public inputs plus user-provided legal firmware/BIOS where required.
