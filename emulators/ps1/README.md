# PlayStation 1 emulator workspace

## Progress: **78%**

Selected implementation: **Beetle PSX HW + Lightrec**.

Canonical detailed status: [ports/beetle-psx-ps5](../../ports/beetle-psx-ps5/README.md).

## Architecture

```text
R3000A
  -> Lightrec
  -> GNU Lightning x86-64 threaded recompiler
  -> ps5rt executable memory
  -> PS5 Zen 2

PS1 GPU
  -> Beetle PSX HW Vulkan
  -> PS5 RADV
  -> PS5 GPU
```

### CPU rule

A final PS5 build with Lightrec disabled does **not** count as complete.

Interpreter execution is accepted only as a temporary diagnostic path.

## Current source

```text
libretro/beetle-psx-libretro@ed87921996c67658d7a70814f73034bbca08786a
```

Our deterministic transform owns the PS5 executable-memory and static-host adaptation. The old PS5_BeetlePSX fork is no longer a build dependency.

## Verified gates

Workflow **36825853078** completed successfully from the current public upstream and produced artifact **11144874334**.

Verified in that run:

- Lightrec + GNU Lightning x86-64;
- threaded compiler;
- Zen 2 / SSE4.1 / AVX2 build;
- Vulkan RHI;
- native Vulkan host negotiation tests;
- AudioOut, DualSense and rumble adapter;
- content/save persistence tests;
- standalone shell compilation.

The next engineering gate is the complete **RADV-linked native PS5 ELF**, followed by title conversion and physical-console validation.
