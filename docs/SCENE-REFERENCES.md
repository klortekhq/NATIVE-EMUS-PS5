# PS5 Scene Engineering References

Last reviewed: 2026-09-30

This is an engineering map, not a popularity list.

## Swordpdf

### PS5SX2
Repository: https://github.com/Swordpdf/PS5SX2

Why it matters:

- native PCSX2 port
- x86-64 PCSX2 recompilers on PS5
- Zen 2-specific compiler tuning
- PS5 executable JIT mappings
- flexible-memory mmap shim
- Vulkan PCSX2 renderer integration
- direct AudioOut implementation
- PS5 frontend / web settings
- storage, CHD/CSO/ZSO and texture-pack work
- crash/logging diagnostics
- GPU readback and submission-pressure debugging

Use as a primary reference for **how a large x86-64 JIT emulator is made native on PS5**.

### Twiso
Repository: https://github.com/Swordpdf/Twiso

Useful for frontend/title launching/storage research, but it is not the architectural model for native emulator cores because it launches external PS2 emulator packages.

## mihawk-99

Profile: https://github.com/mihawk-99

Key repositories:

- PS5_Vulkan
- PS5_Mesa
- PS5_LLVM
- PS5_Dynarmic
- PS5_RPCS3
- PS5_RetroArch
- PS5_LRPS2
- PS5_Mupen64Plus
- PS5_Azahar
- PS5_DeSmuME
- PS5_MAME
- PS5_BeetlePSX
- PS5_BeetleSaturn

### PS5_Mesa
Especially important for our graphics strategy.

Recent PS5-specific work includes:

- RADV PS5 winsys work
- command-submission optimization
- threaded Vulkan command recording
- PS5 synchronization / suspend-point behavior
- RPCS3-driven profiling and optimization

### PS5_LLVM
Critical for RPCS3 and any project embedding LLVM.

Recent fixes address PS4/PS5 ABI alignment behavior affecting LLVM containers and RPCS3 SPU compilation.

### PS5_Dynarmic
Critical for Vita3K / Azahar / Switch-family research.

Contains a PS5-specific executable code-cache path so Dynarmic does not exhaust the title flexible-memory pool.

## BlackBearReloaded

Profile: https://github.com/blackbearreloaded

Key work:

- ProsperoEden
- ps5-native-app-boilerplate
- ps5-opengl
- ProsperoLight
- native controller/runtime research

Useful areas:

- native app startup
- libc / C++ runtime
- TLS
- PS5 services
- title conversion
- filesystem / controller handling
- native Switch-emulator architecture reference

ProsperoEden is especially valuable because its source exposes how a large modern emulator can be adapted into a native PS5 title.

## ps5-payload-dev / John Törnblom

Organization: https://github.com/ps5-payload-dev

Use as a primary public reference for:

- SDK headers/interfaces
- loader behavior
- SDL PS5 work
- networking
- system APIs
- ELF/homebrew examples
- firmware-support infrastructure

Do not fork random prototypes when the SDK already provides a maintained public interface.

## Rufidj / ps5link-sdk

Repository: https://github.com/Rufidj/ps5link-sdk

Useful for:

- linking prospero-clang output into native PS5 titles
- NID import handling
- PLT/GOT
- dynamic SCE tables
- CRT/startup
- libc symbol catalog
- AGC examples
- native GPU/title tooling

Compare this with BlackBear/SharpProspero-style tooling before committing to a permanent project linker pipeline.

## drakmor / VoidWhisper

ShadowMountPlus:
https://github.com/drakmor/ShadowMountPlus

Useful for:

- folder-based native applications
- home-screen integration
- storage/mount behavior
- lifecycle handling

## illusionyy

ps-patch-system:
https://github.com/illusionyy/ps-patch-system

Useful for sandbox/ShellCore patching and runtime integration research.

## EchoStretch

kstuff-lite:
https://github.com/EchoStretch/kstuff-lite

Useful for firmware compatibility and runtime environment tracking.

## LightningMods

etaHEN and related work are relevant to app privileges, jailbreak lifecycle and userland integration.

## Gezine

Profile:
https://github.com/Gezine

Useful repositories include loader, SDK and entry-point research such as Y2JB, elfldr and related tools.

## ArkSama / PHU ecosystem

Profile:
https://github.com/ArkSama

Useful for:

- GhidraProspero
- loader/tool research
- PS5 reverse engineering
- ShellCore/system behavior
- firmware-specific knowledge

## Markus95

Important scene reference for demonstrated ports and experiments.

Caution: not every demonstrated project currently has a public source repository. Treat demonstrations as **evidence of feasibility**, not automatically as reusable source.

## Upstream emulator projects

Always inspect upstream before carrying a scene fork indefinitely.

- PCSX2
- Flycast
- RPCS3
- Vita3K
- Cemu
- xemu
- Xenia
- PPSSPP
- Dolphin
- Mupen64Plus
- Azahar

The ideal long-term port keeps the upstream delta small and isolates PS5 code behind a platform layer.
