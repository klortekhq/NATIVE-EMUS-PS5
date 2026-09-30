# Mupen64Plus PS5 JIT engine gate

Pinned donor:

- `mihawk-99/PS5_Mupen64Plus`
- commit `98ec1019d191fc3c0e1c2d031f2574e95bd8d30d`

This stage deliberately isolates the recompilers from the renderer.

```text
N64 R4300
  -> Mupen64Plus new_dynarec x86-64
  -> ps5rt executable memory
  -> PS5 Zen 2

N64 RSP
  -> ParaLLEl-RSP JIT
  -> ps5rt executable memory
  -> PS5 Zen 2
```

ParaLLEl-RDP is disabled only for this CPU/RSP compile gate. It will be
validated separately against the PS5 Vulkan/RADV path.

No interpreter replaces either JIT in the intended PS5 build.
