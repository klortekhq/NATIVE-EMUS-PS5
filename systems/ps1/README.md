# Sony PlayStation / PS1

## Progress: **86%**

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

Workflow **36833195325** is green from the public upstream pin and validates:

- deterministic PS5 transform;
- current Lightrec/GNU Lightning x86-64 PS5 archive;
- threaded recompiler;
- Beetle Vulkan RHI;
- native Vulkan environment/provider/presenter host tests;
- PS5 `VK_KHR_display` surface selection logic;
- AudioOut/input/rumble bridge;
- local CUE/CHD/PBP/ISO/CCD/TOC/M3U/PS-X EXE content policy;
- native 2048/2336/2352-byte per-track sector widths by LBA;
- validated local multi-disc M3U resolution with network/nested playlists rejected until VFS is ready;
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

The 2026-10-01 hardening pass added an immutable RADV bundle receipt, native
mixed-mode sector metadata, explicit region-aware BIOS policy and the shared
native FSELF finalizer with PS1's AGC import stubs.

Workflow **37160191512** subsequently completed the exact pinned RADV build,
standalone title link and native finalization on the current public Beetle /
Lightrec path. The run produced a linked PIE plus `eboot.elf` and
`eboot.bin`; this advances the reproducible graphics/title-link gates but is
still **not** evidence that the title boots or renders correctly on physical
PS5 hardware.

## Remaining gates

1. physical PS5 boot of the finalized title;
2. first-frame / Vulkan presentation validation on hardware;
3. BIOS/OpenBIOS and legal test-content validation;
4. mixed-mode/game/media/save compatibility pass;
5. SERVER-EMUS/SMB seek-read validation and read-ahead tuning on LAN;
6. performance tuning without dropping Lightrec.

## Workspaces

- [Detailed PS1 port](../../ports/beetle-psx-ps5/README.md)
- [Engine workspace](../../emulators/beetle-psx/README.md)
- [Shared PS5 runtime](../../runtime/README.md)

### Latest gate

Workflow **36835002466** is green. Slot-0 save states are persisted atomically,
corrupt states are rejected before unserialize, and multi-disc changes use
Beetle's registered disk-control interface after the frame. The Lightrec x86-64
PS5 engine was rebuilt in the same run as artifact **11148931941**.


### Full native title evidence — 2026-10-03

Workflow **37160191512** completed successfully and uploaded artifact
**11287755203** (`ps1-beetle-lightrec-radv-native-title`).

Artifact ZIP SHA-256:

```text
b1ee45cc9afc153dc23d166c11a5b113d8924af54f46029bc606d8acc1653e50
```

Key output hashes recorded by CI:

```text
f0907077558af6a463af8feb0eef96a5acf7f0ed6c3de8be80db244d6b1cb8d6  beetle_psx_hw_ps5_pie.elf
81fe67d5b06464667f75310591c516c3db655e12fc630c20bc6991ae99967326  native/eboot.elf
bcafaed0f883f9499f7754f88ff370b78872711a21b3c53d7562001793d9c4a2  native/eboot.bin
```

The run also rechecked that the Lightrec/GNU Lightning x86-64 recompiler
survives the final engine build. Hardware execution remains the next gate.
