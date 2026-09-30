# Azahar / Nintendo 3DS

## Key dependency

Dynarmic is the central reason this target aligns with the shared PS5 runtime.

The PS5_Dynarmic code-cache work is directly relevant.

## Goals

- native Dynarmic x86-64 execution
- Vulkan renderer
- PS5 input/audio/VFS
- standalone frontend path

## Next milestone

Inventory PS5-specific Dynarmic and Vulkan changes required by the current scene port and separate them from libretro/frontend glue.


## Shared Dynarmic PS5 gate

- [x] PS5 Xbyak executable-code allocator identified in Mihawk's fork
- [x] allocator retargeted to shared `ps5rt_exec_*`
- [x] final CPU policy: Dynarmic x86-64 JIT, not full interpretation
- [ ] Azahar core cross-compile against the shared allocator
- [ ] physical PS5 JIT/fastmem validation
