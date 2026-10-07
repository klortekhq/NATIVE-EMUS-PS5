# Game Boy Advance

**Catalog group:** M8 baseline  
**Primary emulator/core candidates:** mGBA  
**Repository state:** Scaffold — primary mGBA core cross-build verified

## Native PS5 objective

Keep the emulation core direct and upstream-shaped, then replace only the host/platform services required for PS5.

Expected host integration areas:

- executable memory/JIT when the core uses dynamic recompilation;
- Vulkan/RADV or the most appropriate native renderer path;
- `libSceAudioOut` backend;
- DualSense / keyboard / mouse where relevant;
- PS5 VFS for internal, USB/M.2 and optional network storage;
- deterministic logging, crash reporting and reproducible packaging.

## Current note

Primary mGBA revision `c3c8e5e813f245028de118a56734e1dc0f35ce2a` is pinned under MPL-2.0 and its direct static ARM7TDMI core library cross-builds with the public PS5 toolchain. No standalone title or hardware boot is claimed.

## Architecture rule

No Linux/Wine dependency is accepted as the project's native architecture. RetroArch/libretro ports may be used as valuable PS5 engineering references, but the long-term target remains a clean native port.

## Next step

Link the verified direct mGBA core to the shared `corehost + ps5rt` lifecycle, software framebuffer, AudioOut, DualSense and VFS path; then produce the smallest standalone PS5 title before any hardware-status promotion.
