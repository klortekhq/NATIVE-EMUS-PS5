# Project Status

Last reviewed: 2026-09-30

This document distinguishes **verified public scene progress** from **our repository implementation state**.

## Repository implementation state

At initial publication, this repository is a project hub and engineering workspace. It does **not** yet claim that all listed emulators have been built from this repository.

| Target | Repo state | External evidence / useful reference |
|---|---|---|
| PCSX2 / PS2 | Research / reference | Swordpdf/PS5SX2 demonstrates native PCSX2 recompilers + Vulkan on PS5 |
| Flycast / Dreamcast | Research | Upstream is active and has standalone Vulkan + x86-64 dynarec |
| RPCS3 / PS3 | Research / fast-moving external work | mihawk-99/PS5_RPCS3, PS5_LLVM and PS5_Mesa show active PS5-specific optimization |
| Vita3K / Vita | Research | Upstream active; Dynarmic PS5 work exists separately in mihawk-99/PS5_Dynarmic |
| Cemu / Wii U | Research | Upstream has Vulkan and PPC recompiler; desktop UI/platform removal is a major task |
| xemu / Xbox | Research | Vulkan path exists upstream; QEMU-derived platform dependencies remain substantial |
| Xenia / Xbox 360 | Research | x64 CPU backend is host-appropriate; Vulkan/GPU adaptation is the main challenge |
| Switch / Eden-derived | Reference research | ProsperoEden source provides a native PS5 implementation reference |
| PPSSPP / PSP | External port research | Public PS5-specific repository exists |
| Mupen64Plus / N64 | External port research | mihawk-99 PS5 work and upstream dynarec are useful references |
| Azahar / 3DS | External port research | PS5_Dynarmic and PS5 scene work are directly relevant |
| Dolphin / GC/Wii | External port research | PS5 RetroArch work demonstrates JIT/Vulkan feasibility |

## Platform readiness

| Component | State | Notes |
|---|---|---|
| Native x86-64 JIT memory | Proven externally | PS5SX2 and PS5_Dynarmic contain working approaches |
| Vulkan on PS5 | Proven externally / rapidly improving | Mesa/RADV PS5 work is active |
| PS5-specific LLVM ABI | Active external development | Important RPCS3 fixes landed in PS5_LLVM |
| AudioOut | Proven externally | PS5SX2 uses direct libSceAudioOut |
| DualSense | Proven externally | Multiple homebrew projects expose working implementations |
| Keyboard/mouse | Partial / firmware-sensitive | PS5SX2 reports library-loading differences on newer firmwares |
| Persistent shader cache | Proven externally | PS5 Vulkan projects use it |
| Native title tooling | Proven externally | BlackBear tooling, ps5link-sdk, SharpProspero references |
| External storage | Proven externally | ProsperoEden / ShadowMountPlus / other homebrew provide reference behavior |
| Suspend/resume | Needs per-port validation | Still a platform-specific risk |
| Multi-controller | Proven in current scene work | Recent ProsperoEden changes support one controller per signed-in user |

## Current engineering priority

1. stabilize shared runtime contracts;
2. use PCSX2/PS5SX2 as the strongest x86-64/JIT reference;
3. use PS5_Mesa/PS5_LLVM/PS5_Dynarmic as reusable platform research;
4. begin Flycast as the first clean standalone greenfield port;
5. track RPCS3 aggressively because external PS5 work is advancing daily;
6. prototype Vita3K on the shared Dynarmic/JIT path;
7. move to Cemu after runtime and Vulkan are sufficiently reusable;
8. keep xemu/Xenia as deeper platform projects rather than superficial ports.
