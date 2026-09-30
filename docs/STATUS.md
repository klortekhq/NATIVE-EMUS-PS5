# Project Status

Last reviewed: 2026-09-30

This document distinguishes **verified public scene progress** from **our repository implementation state**.

## Repository coverage

- **60 system / hardware targets** are represented under `systems/`.
- **43** are the recovered M8 baseline; the catalog was expanded with every additional system explicitly discussed, including board-specific arcade targets.
- emulator/core workspaces are separated from the machine catalog so one clean engine can serve multiple systems.
- the shared `ps5rt` layer now contains public contracts plus native PS5 implementations for application lifecycle, local random-access I/O, software VideoOut, AudioOut and DualSense input; JIT/TLS/Vulkan work remains on the modern-emulator track.
- M8 and earlier recovered artifacts remain documented separately under `legacy/` and `artifacts/`.

A directory or contract does **not** mean that the corresponding emulator is already built or playable from this repository. Status claims remain evidence-based.

## Repository-native PS5 cross-builds

The following ports have completed a reproducible cross-build with the public `ps5-payload-dev` SDK in GitHub Actions. **Cross-built is not the same as hardware-playable**: these binaries still require validation on an actual jailbroken PS5.

| System | Engine | PS5 output | Evidence |
|---|---|---|---|
| NES | FCEUmm | `nes_fceumm_pie.elf` | workflow 36716495397 / green |
| SNES | Snes9x | `snes9x_pie.elf` | workflow 36716495397 / green |
| SG-1000 | Genesis Plus GX | `sg1000_genesis_plus_gx_pie.elf` | workflow 36716495397 / green |
| Master System | Genesis Plus GX | `mastersystem_genesis_plus_gx_pie.elf` | workflow 36716495397 / green |
| Game Gear | Genesis Plus GX | `gamegear_genesis_plus_gx_pie.elf` | workflow 36716495397 / green |
| Mega Drive / Genesis | Genesis Plus GX | `megadrive_genesis_plus_gx_pie.elf` | workflow 36716495397 / green |

Artifacts from workflow 36716495397 include:

- `native-nes-fceumm-ps5-pie`
- `native-snes-snes9x-ps5-pie`
- `native-sega-genesis-plus-gx-ps5-pie`

The Sega artifact contains all four cartridge-family ELFs.

## Modern / JIT-heavy targets

| Target | Repo state | External evidence / useful reference |
|---|---|---|
| PCSX2 / PS2 | Reference integration research | Swordpdf/PS5SX2 demonstrates native PCSX2 recompilers + Vulkan on PS5 |
| Flycast / Dreamcast | Port plan | Upstream standalone x86-64 dynarec + Vulkan is a strong fit for ps5rt |
| RPCS3 / PS3 | Fast-moving external work | mihawk-99/PS5_RPCS3, PS5_LLVM and PS5_Mesa contain active PS5-specific work |
| Vita3K / Vita | Port plan | PS5_Dynarmic work directly addresses executable code-cache needs |
| Cemu / Wii U | Port plan | PPC recompiler + Vulkan; desktop UI separation remains a major task |
| xemu / Xbox | Platform research | QEMU-derived machine model makes host isolation broader |
| Xenia / Xbox 360 | Platform/GPU research | x64 backend fits the host; GPU/EDRAM/Vulkan adaptation is the larger problem |
| Switch / Eden-derived | Reference research | ProsperoEden is a public native PS5 implementation reference |
| PPSSPP / PSP | External port research | Public PS5-specific work exists |
| Mupen64Plus / N64 | External port research | mihawk-99 PS5 work and upstream dynarec are useful references |
| Azahar / 3DS | External port research | PS5_Dynarmic and PS5 scene work are directly relevant |
| Dolphin / GC/Wii | External port research | Existing PS5 work demonstrates JIT/Vulkan feasibility |
| DeSmuME / NDS | External port research | mihawk-99/PS5_DeSmuME provides a donor/reference |
| Beetle PSX / Saturn | External port research | mihawk-99 PS5 ports exist |
| MAME / VICE | External port research | mihawk-99 PS5 ports exist |

## Retro / portable target families

Tracked workspaces now also cover:

- Mesen 2, Snes9x, SameBoy and mGBA;
- Genesis Plus GX and PicoDrive;
- Mednafen/Beetle family systems;
- FinalBurn Neo and NeoCD;
- Stella, Atari800, ProSystem and Virtual Jaguar;
- blueMSX, Fuse and Gearcoleco;
- FreeIntv and PUAE.

These are especially useful for hardening AudioOut, DualSense, VFS, packaging and lifecycle before those services are consumed by the heaviest JIT/Vulkan emulators.

## Platform readiness

| Component | Repository state | External state / notes |
|---|---|---|
| public runtime contract | **Implemented + evolving** | C++20 contracts plus native PS5 app/audio/input/io/video backends are in `runtime/` |
| executable/JIT memory | interface defined | working approaches exist in PS5SX2 and PS5_Dynarmic |
| flexible/direct/pooled memory | interface defined | implementation migration still pending |
| TLS | interface defined | compiler-rt emutls and PS5-specific shims need implementation/evaluation |
| thread policy | interface defined | explicit stack handling is proven necessary by PS5SX2 |
| AudioOut | **native backend implemented** | 48 kHz PS5 output with stateful source-rate conversion; cross-build proven |
| DualSense | **native backend implemented** | controller 0 polling is cross-build proven; rumble/multi-user remain to migrate |
| VFS/storage | **local random-access backend implemented** | /data and USB path policy active; SMB/network migration remains pending |
| Vulkan bootstrap | interface defined | Mesa/RADV PS5 work is active and rapidly improving |
| PS5-specific LLVM ABI | external active development | critical RPCS3 fixes exist in PS5_LLVM |
| native title tooling | research mapped | BlackBear tooling, ps5link-sdk and SharpProspero are tracked |

## Current engineering lanes

1. **runtime implementation:** migrate proven M8/Swordpdf/Mihawk platform solutions behind the new ps5rt contract;
2. **Flycast:** first greenfield standalone modern port;
3. **RPCS3:** continuously track and understand PS5_RPCS3 / PS5_LLVM / PS5_Mesa rather than duplicating fixes;
4. **Vita3K/Azahar:** converge on the common PS5_Dynarmic/JIT solution;
5. **portable cores:** use Snes9x/SameBoy/mGBA/Genesis Plus GX/Stella-class ports to validate audio/input/VFS/packaging;
6. **Cemu/Dolphin/PPSSPP/N64/NDS/PS1/Saturn:** consolidate known scene fixes into standalone-native designs;
7. **xemu/Xenia/MAME/Jaguar/Amiga:** maintain dedicated deeper-host tracks.
