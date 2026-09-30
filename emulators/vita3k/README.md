# Vita3K / PlayStation Vita

## Why it fits the shared runtime

Vita3K uses Dynarmic and Vulkan, which aligns well with current PS5 scene work.

Primary reusable reference:

- mihawk-99/PS5_Dynarmic executable code cache

## Desired architecture

```text
ARM guest
  -> Dynarmic
  -> PS5 executable code cache
  -> Zen 2

GXM
  -> Vita3K Vulkan renderer
  -> PS5 Mesa/RADV
```

## Keep from upstream

- Vita kernel/HLE
- Dynarmic integration
- GXM translation
- Vulkan renderer
- module loading
- shader translation

## Replace/adapt

- SDL/windowing
- audio backend
- controller backend
- filesystem
- firmware/user-data locations
- executable-memory allocator

## Next milestone

Compile Vita3K core for PS5 with Dynarmic using ps5rt::jit and create a Vulkan device without desktop window dependencies.


## Shared Dynarmic PS5 gate

- [x] PS5 Xbyak executable-code allocator identified in Mihawk's fork
- [x] allocator retargeted to shared `ps5rt_exec_*`
- [x] final CPU policy: Dynarmic x86-64 JIT, not full interpretation
- [ ] Vita3K core cross-compile against the shared allocator
- [ ] physical PS5 JIT/fastmem validation
