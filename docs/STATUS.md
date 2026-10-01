# Project Status

Last reviewed: 2026-10-01

This document distinguishes **verified public scene progress** from **our repository implementation state**.

## Repository coverage

- **60 system / hardware targets** are represented under `systems/`.
- **43** are the recovered M8 baseline; the catalog was expanded with every additional system explicitly discussed, including board-specific arcade targets.
- emulator/core workspaces are separated from the machine catalog so one clean engine can serve multiple systems.
- the shared `ps5rt` layer now contains public contracts plus native PS5 implementations for application lifecycle, local/SMB/`emus://` random-access I/O, software VideoOut, AudioOut and DualSense input; JIT/TLS/Vulkan work remains on the modern-emulator track.
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
| Flycast / Dreamcast | **rec-x64 engine cross-build green** | run 36845566519 cross-built the real SH4 rec-x64 + ARM7/DSP x64 engine and PS5 vmem adapter; artifact 11153815261. Next gate: exact-capacity RW/RX execution + fastmem on physical PS5, then native services/Vulkan |
| RPCS3 / PS3 | **canonical PPU/SPU engine cross-build green** | run 36855520217 cross-built JITASM + SPU AsmJit + PPU LLVM + SPU LLVM from canonical RPCS3 and uploaded artifact 11158467437. A 2 GiB sparse-JIT ps5rt probe also cross-builds (artifact 11159199014); physical execution is the next gate |
| Vita3K / Vita | **shared A32/A64 x64 JIT gate green** | `libdynarmic_ps5.a` cross-build is green; Vita3K integration is next |
| Cemu / Wii U | Port plan | PPC recompiler + Vulkan; desktop UI separation remains a major task |
| xemu / Xbox | Platform research | QEMU-derived machine model makes host isolation broader |
| Xenia / Xbox 360 | Platform/GPU research | x64 backend fits the host; GPU/EDRAM/Vulkan adaptation is the larger problem |
| Switch / Eden-derived | **shared A32/A64 x64 JIT gate green** | Dynarmic PS5 archive is reproducible; ProsperoEden remains the native platform reference |
| PPSSPP / PSP | External port research | Public PS5-specific work exists |
| Mupen64Plus / N64 | **CPU + RSP JIT transform preserved** | exact historical donor evidence is retained; current CI falls back to a frozen ps5rt transform-contract regression when that donor is not publicly retrievable |
| Azahar / 3DS | **canonical A32 sparse x64 JIT gate green** | canonical Azahar `662d412...` + its Dynarmic `e77b1ba...` cross-build with sparse fixed-address reserve, incremental direct-memory commits and W^X; artifact 11165471584. Physical execution/core integration are next |
| Dolphin / GC/Wii | External port research | Existing PS5 work demonstrates JIT/Vulkan feasibility |
| DeSmuME / NDS | External port research | historical Mihawk PS5 work is retained as evidence; live source availability must be re-audited before reuse |
| Beetle PSX / Saturn | Upstream-first / historical donor evidence | PS1 now builds from public Beetle upstream; old Mihawk ports are historical feasibility evidence only |
| MAME / VICE | External port research | historical Mihawk PS5 work is evidence only until public source provenance is re-established |

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
| executable/JIT memory | **native backend + sparse fixed-address arena implemented** | JIT shared memory, direct fallback, dual RW/RX aliases and incremental direct-memory commits inside reserved VA now exist. Flycast rec-x64 and RPCS3 PPU/SPU engines cross-build; the RPCS3 2 GiB sparse arena probe cross-builds with Prospero and awaits physical validation |
| flexible/direct/pooled memory | **flexible/direct + sparse arena implemented; pooled pending** | virtual ranges, shared/direct mappings, fixed mapping helpers and sparse fixed-offset direct-memory chunks are in the PS5 backend |
| TLS | interface defined | compiler-rt emutls and PS5-specific shims need implementation/evaluation |
| thread policy | interface defined | explicit stack handling is proven necessary by PS5SX2 |
| AudioOut | **native backend implemented** | 48 kHz PS5 output with stateful source-rate conversion; cross-build proven |
| DualSense | **native backend implemented** | signed-in-user controller enumeration, sticks/triggers/buttons and rumble are implemented; keyboard/mouse PS5 backends remain pending |
| VFS/storage | **local + SMB + EMUS random-access implemented** | /data, USB/external discovery and saves are active; native `emus://` uses persistent HTTP connections, Range/ETag, anchored descriptor sidecars and one controlled reconnect on native transport failure. Physical-PS5/LAN validation and measured read-ahead tuning remain |
| Vulkan bootstrap | interface defined + PS1 provider/presenter implemented | historical Mihawk RADV pins are currently unavailable; active public references include `mpereiraesaa/ps5-vulkan` and BlackBear's PS5 graphics/tooling work, but no ABI migration is assumed |
| PS5-specific LLVM ABI | **canonical defect reproduced + local deterministic shim green** | SCE over-alignment failure is proven against RPCS3's canonical LLVM pin; the marker-checked `__SCE__` shim enables real PPU/SPU object compilation without depending on unavailable historical forks |
| native title tooling | research mapped | BlackBear tooling, ps5link-sdk and SharpProspero are tracked |

## Current engineering lanes

1. **runtime implementation:** migrate proven M8/Swordpdf/Mihawk platform solutions behind the new ps5rt contract;
2. **Flycast:** first greenfield standalone modern port;
3. **RPCS3:** preserve verified PS5-specific findings while re-auditing live public provenance before importing any old Mihawk dependency;
4. **Vita3K/Azahar:** keep each emulator on its canonical pinned Dynarmic revision while sharing the proven ps5rt executable-memory ABI; Azahar's current A32 sparse-JIT engine is now cross-build green;
5. **portable cores:** use Snes9x/SameBoy/mGBA/Genesis Plus GX/Stella-class ports to validate audio/input/VFS/packaging;
6. **Cemu/Dolphin/PPSSPP/N64/NDS/PS1/Saturn:** consolidate known scene fixes into standalone-native designs;
7. **xemu/Xenia/MAME/Jaguar/Amiga:** maintain dedicated deeper-host tracks.


### PlayStation / PS1 — 83%

- Selected core: Beetle PSX HW.
- Required CPU backend: Lightrec + GNU Lightning x86-64.
- Lightrec + GNU Lightning x86-64 engine and standalone Vulkan host are green in CI; full pinned RADV app link + physical PS5 validation remain.
- Canonical source is `libretro/beetle-psx-libretro@ed87921996c67658d7a70814f73034bbca08786a`; Mihawk's older PS5_BeetlePSX is historical feasibility evidence only.
- Deterministic transform is green from public upstream; workflow 36833195325 also validates local multi-disc M3U and per-track native sector widths.
- Transform re-enables Lightrec on PS5 and supplies its TLSF code pool through `ps5rt_exec_allocate()`.
- Native PS5 Lightrec x86-64 engine cross-build is **green**; workflow 36835002466 completed successfully and uploaded artifact 11148931941.
- Binary inspection confirms `lightrec.o`, `recompiler.o`, `lightning.o`, `jit_memory.o`, `lightrec_execute`, `_jit_set_code`, and references to `ps5rt_exec_allocate/release`.
- Engine SHA-256: `b709412d7cc3dbe815fcde63dc6daddfdf994fe96998a12ed148cc857df2deb2`.
- Vulkan host/native service adapters, local media, guarded save states and Beetle-native multi-disc switching are implemented.
- Native `emus://` random access is implemented with HEAD/Range/ETag, and descriptor-relative CUE/CCD/TOC/M3U sidecars are transported through an opaque catalog anchor without exposing NAS paths. The server side rejects library and symlink escapes.
- Workflow `36844576020` is green for the current PS1 Lightrec lane, including the Prospero compile of the EMUS backend and remote-content regression coverage.
- The complete RADV title link remains a separate open gate: its exact historical PS5_Vulkan/PS5_Mesa/PS5_PayloadSDK pins are currently unavailable, so CI now reports that provenance gate as blocked instead of mislabeling it as a code regression. Physical-console validation also remains.

See `systems/ps1/README.md` for the weighted percentage.
