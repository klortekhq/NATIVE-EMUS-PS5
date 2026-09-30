# Flycast / Dreamcast · Naomi · Atomiswave

## Why this is a first-class target

Flycast is active, standalone-capable, uses Vulkan and has native dynarec paths suitable for an x86-64 host.

## Desired architecture

```text
SH4 guest
  -> Flycast x86-64 dynarec
  -> ps5rt::jit
  -> Zen 2

PowerVR2
  -> Flycast Vulkan renderer
  -> PS5 Mesa/RADV
```

## Keep from upstream

- SH4 dynarec
- scheduler
- Dreamcast/Naomi/Atomiswave machine code
- Vulkan renderer
- shader logic
- save state and VM logic

## Replace/adapt

- desktop window system
- SDL/platform input
- desktop audio
- filesystem paths
- JIT allocation
- process lifecycle

## Next milestone

A headless PS5 build that reaches Dreamcast BIOS with:

- x86-64 dynarec enabled
- Vulkan device creation
- AudioOut initialized
- DualSense mapped
