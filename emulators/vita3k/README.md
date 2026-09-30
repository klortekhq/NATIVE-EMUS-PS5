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
