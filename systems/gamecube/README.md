# Nintendo GameCube

**Catalog group:** M8 baseline  
**Primary emulator/core candidates:** Dolphin  
**Repository state:** Research / port planning

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

x86-64 JIT + Vulkan.

## Architecture rule

No Linux/Wine dependency is accepted as the project's native architecture. RetroArch/libretro ports may be used as valuable PS5 engineering references, but the long-term target remains a clean native port.

## Next step

Audit the selected upstream core against `runtime/`, classify its CPU/JIT, graphics, audio, input, filesystem and threading requirements, then create the smallest bootable PS5 target before adding a rich frontend.
