# PPSSPP native PS5 engine

Pinned upstream: `hrydgard/ppsspp@fa50bb1976065c4f8b1b47af227d367fe9771555` (v1.20.4).

This target cross-builds PPSSPP's **native AMD64 MIPS recompilers** for PS5. The build is rejected unless the resulting `libCore.a` contains both the classic x86 JIT and X64 IR JIT object code.

## Architecture

```text
PSP MIPS guest
  -> PPSSPP x86-64 JIT / X64 IR JIT
  -> ps5rt executable memory
  -> PS5 Zen 2
```

The current gate produces the Common/Core engine archives. The next gate is the native PS5 application lifecycle plus Vulkan/RADV renderer integration.

No interpreter-only build is accepted as the final PS5 backend.
