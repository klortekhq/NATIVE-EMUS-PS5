# Sony PlayStation / PS1

## Progress: **78%**

> Native standalone PS5 target. The canonical detailed engineering status lives in [ports/beetle-psx-ps5](../../ports/beetle-psx-ps5/README.md).

## Selected engine

**Beetle PSX HW + Lightrec**

Canonical source:

```text
libretro/beetle-psx-libretro@ed87921996c67658d7a70814f73034bbca08786a
```

The older `mihawk-99/PS5_BeetlePSX` work is retained only as historical feasibility evidence. The current port no longer depends on that unavailable donor.

## CPU architecture

```text
R3000A guest
  -> Lightrec
  -> GNU Lightning x86-64
  -> ps5rt executable code pool
  -> PS5 Zen 2
```

The final PS5 build **must use the recompiler**. Interpreter execution is diagnostic only.

## Current verified state

Workflow **36825853078** is green from the public upstream pin and validates:

- deterministic PS5 transform;
- current Lightrec/GNU Lightning x86-64 PS5 archive;
- threaded recompiler;
- Beetle Vulkan RHI;
- native Vulkan environment/provider/presenter host tests;
- PS5 `VK_KHR_display` surface selection logic;
- AudioOut/input/rumble bridge;
- local CUE/CHD/PBP/ISO/CCD/TOC/M3U/PS-X EXE content policy;
- atomic SRAM persistence;
- standalone native PS1 shell compilation.

Current engine artifact:

```text
GitHub Actions artifact: 11144874334
native-beetle-psx-hw-lightrec-x64-ps5-engine
ZIP SHA-256:
0022701a3d1be329f3698b8eaaeda9d6a1cf3655995dc71aa68b0c6d4589c11f
```

The archive contains `lightrec.o`, `recompiler.o`, GNU Lightning and the Vulkan RHI, compiled for Zen 2 with SSE4.1/AVX2.

## Graphics

```text
Beetle PSX HW Vulkan
  -> native PS5 Vulkan negotiation/provider
  -> PS5 RADV linked into title
  -> VK_KHR_display / VideoOut
  -> PS5 GPU
```

The driver is linked into the title; the PS5 port does not rely on a desktop Vulkan loader.

## Remaining gates

1. finish the full standalone RADV-linked ELF;
2. resolve static frontend support symbols and title-link imports;
3. native title/FSELF conversion;
4. physical PS5 boot;
5. BIOS/OpenBIOS and legal test-content validation;
6. game/media/save compatibility pass;
7. performance tuning without dropping Lightrec.

## Workspaces

- [Detailed PS1 port](../../ports/beetle-psx-ps5/README.md)
- [Engine workspace](../../emulators/beetle-psx/README.md)
- [Shared PS5 runtime](../../runtime/README.md)
