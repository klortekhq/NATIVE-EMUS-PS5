# Xenia / Xbox 360

## Why it is plausible

Xenia already has an x64 CPU backend, which is architecturally appropriate for the PS5 host CPU.

The larger challenge is GPU semantics and host integration.

## Desired architecture

```text
PowerPC/Xbox 360 guest
  -> Xenia x64 backend
  -> Zen 2

Xenos
  -> Xenia Vulkan path
  -> PS5 Mesa/RADV
```

## Critical areas

- executable memory
- x64 backend platform assumptions
- EDRAM behavior
- shader translation
- Vulkan feature/format requirements
- threading
- filesystem
- audio/input
- frontend removal

## Next milestone

Audit Vulkan extension/feature requirements against the current PS5 Mesa/RADV implementation and classify blockers before touching UI code.
