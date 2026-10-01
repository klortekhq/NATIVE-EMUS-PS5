# Project Status

Last reviewed: 2026-10-01

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
| NES | FCEUmm | `nes_fceumm_pie.elf` | workflow 36748819649 / green after corehost migration |
| SNES | Snes9x | `snes9x_pie.elf` | workflow 36748819649 / green after corehost migration |
| SG-1000 | Genesis Plus GX | `sg1000_genesis_plus_gx_pie.elf` | workflow 36748819649 / green after corehost migration |
| Master System | Genesis Plus GX | `mastersystem_genesis_plus_gx_pie.elf` | workflow 36748819649 / green after corehost migration |
| Game Gear | Genesis Plus GX | `gamegear_genesis_plus_gx_pie.elf` | workflow 36748819649 / green after corehost migration |
| Mega Drive / Genesis | Genesis Plus GX | `megadrive_genesis_plus_gx_pie.elf` | workflow 36748819649 / green after corehost migration |
| Intellivision | FreeIntv | `freeintv_pie.elf` | workflow 36748819649 / green |
| Odyssey2 / Videopac | O2EM | `o2em_pie.elf` | workflow 36748819649 / green |

Current cross-build evidence is workflow **36748819649**. It validates the shared `corehost + ps5rt` runner after migration and produces the NES, SNES, four Genesis Plus GX variants, FreeIntv and O2EM PS5 PIE outputs. The earlier workflow 36716495397 remains useful as pre-migration evidence.

Host-side static-link/lifecycle validation is green in workflow **36749064626** for FreeIntv and O2EM, alongside the Flycast, Dynarmic, Mupen64Plus, Dolphin and PPSSPP transformation checks.

Dynarmic A32+A64 x86-64 JIT cross-build evidence: workflow **36760157664**.\n\n## Modern / JIT-heavy targets

| Target | Repo state | External evidence / useful reference |
|---|---|---|
| PCSX2 / PS2 | Reference integration research | Swordpdf/PS5SX2 demonstrates native PCSX2 recompilers + Vulkan on PS5 |
| Flycast / Dreamcast | **rec-x64/JIT transform green** | deterministic PS5 fastmem + dual-view JIT transform is green in workflow 36749064626; engine cross-build is the next gate |
| RPCS3 / PS3 | Fast-moving external work | mihawk-99/PS5_RPCS3, PS5_LLVM and PS5_Mesa contain active PS5-specific work |
| Vita3K / Vita | **shared A32/A64 x64 JIT gate green** | `libdynarmic_ps5.a` cross-build is green; Vita3K integration is next |
| Cemu / Wii U | Port plan | PPC recompiler + Vulkan; desktop UI separation remains a major task |
| xemu / Xbox | Platform research | QEMU-derived machine model makes host isolation broader |
| Xenia / Xbox 360 | Platform/GPU research | x64 backend fits the host; GPU/EDRAM/Vulkan adaptation is the larger problem |
| Switch / Eden-derived | **shared A32/A64 x64 JIT gate green** | Dynarmic PS5 archive is reproducible; ProsperoEden remains the native platform reference |
| PPSSPP / PSP | External port research | Public PS5-specific work exists |
| Mupen64Plus / N64 | **CPU + RSP JIT transform green** | mihawk-99 donor allocators retarget cleanly to ps5rt in workflow 36749064626 |
| Azahar / 3DS | **shared A32/A64 x64 JIT gate green** | `libdynarmic_ps5.a` now cross-builds with both JIT frontends |
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
| executable/JIT memory | **native backend implemented + transform consumers green** | JIT shared memory, direct fallback and dual RW/RX aliases exist; Flycast/Dynarmic/Mupen transforms are CI-green |
| flexible/direct/pooled memory | **flexible/direct implemented; pooled pending** | virtual ranges, shared/direct mapping and fixed mapping helpers are in the PS5 backend |
| TLS | interface defined | compiler-rt emutls and PS5-specific shims need implementation/evaluation |
| thread policy | interface defined | explicit stack handling is proven necessary by PS5SX2 |
| AudioOut | **native backend implemented** | 48 kHz PS5 output with stateful source-rate conversion; cross-build proven |
| DualSense | **native backend implemented** | signed-in-user controller enumeration, sticks/triggers/buttons and rumble are implemented; keyboard/mouse PS5 backends remain pending |
| VFS/storage | **local random-access + directory/storage backend implemented** | /data, USB/external discovery and save directories active; SMB/network migration remains pending |
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


### PlayStation / PS1 — 80%

- Selected core: Beetle PSX HW.
- Required CPU backend: Lightrec + GNU Lightning x86-64.
- Lightrec + GNU Lightning x86-64 engine and standalone Vulkan host are green in CI; full pinned RADV app link + physical PS5 validation remain.
- Canonical source is `libretro/beetle-psx-libretro@ed87921996c67658d7a70814f73034bbca08786a`; Mihawk's older PS5_BeetlePSX is historical feasibility evidence only.
- Deterministic transform is green from public upstream; workflow 36833195325 also validates local multi-disc M3U and per-track native sector widths.
- Transform re-enables Lightrec on PS5 and supplies its TLSF code pool through `ps5rt_exec_allocate()`.
- Native PS5 Lightrec x86-64 engine cross-build is **green**; current public-upstream workflow 36833195325 completed successfully.
- Binary inspection confirms `lightrec.o`, `recompiler.o`, `lightning.o`, `jit_memory.o`, `lightrec_execute`, `_jit_set_code`, and references to `ps5rt_exec_allocate/release`.
- Engine SHA-256: `b709412d7cc3dbe815fcde63dc6daddfdf994fe96998a12ed148cc857df2deb2`.
- Vulkan host/native service adapters and local media handling are implemented; complete pinned RADV title link, packaging evidence and physical-console validation remain.

See `systems/ps1/README.md` for the weighted percentage.
