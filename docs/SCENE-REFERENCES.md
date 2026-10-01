# PS5 Scene Engineering References

Last reviewed: 2026-10-01

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

Audit note (2026-10-01): `Swordpdf/PS5SX2` remains public and active. The
latest checked main revision is `b5e4d310fe23fea25a284601d4a35dbb2f1cfd81`
(PS5SX2 1.51 merge). Consume individual platform fixes deliberately rather
than rebasing unrelated emulator code onto its frontend.

### Twiso
Repository: https://github.com/Swordpdf/Twiso

Useful for frontend/title launching/storage research, but it is not the architectural model for native emulator cores because it launches external PS2 emulator packages.

## mihawk-99

Profile: https://github.com/mihawk-99

### Availability warning — 2026-10-01

The previously tracked Mihawk repositories below are **historical engineering
references, not currently reliable live dependencies**. Fresh GitHub API
lookups and CI clone/codeload attempts on 2026-10-01 return unavailable/404 for
the exact PS5_Vulkan, PS5_Mesa, PS5_PayloadSDK, PS5_Dynarmic and
PS5_Mupen64Plus sources used by our old pins.

Rules for this repository:

- preserve the exact historical revisions and successful build evidence;
- never silently substitute a same-named fork or moving branch;
- when an exact donor is unavailable, run our frozen transform-contract tests
  and report provenance as blocked;
- only restore an integration gate when the exact source is retrievable again
  or after an explicit, reviewed migration to a new public upstream.

Historically tracked repositories:

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

## mpereiraesaa / ps5-vulkan

Repository: https://github.com/mpereiraesaa/ps5-vulkan

This is a **currently public, active native PS5 Vulkan reference** and is worth
tracking independently from the historical Mihawk RADV stack. Latest checked
revision on 2026-10-01 is
`e6d3c3f82f3c8a574e28c174b15b287f9c0ecc6d`.

Useful areas include native GPU/WSI evidence, PS5 payload-SDK integration,
shader/compiler work and recent RADV/DXVK physical-display investigations.
It is not assumed ABI-compatible with our frozen PS1 RADV bundle contract:
migration requires an explicit link/runtime audit and fresh physical evidence.

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

## Phi1ow / mkwii-ps5

Repository: https://github.com/Phi1ow/mkwii-ps5

This is **not a generic Dolphin port**. It is a native PS5 build of a statically recompiled Mario Kart Wii PAL executable based on WiiCompiled.

Why it matters to NATIVE-EMUS-PS5:

- concrete PS5 guest-address-space management with fixed, no-overwrite virtual reservations;
- direct-memory backing and multiple fixed aliases for Wii guest regions;
- documented split between memory type 11 (general cached CPU memory) and type 12 (CPU/GPU shared memory), corroborated by SharpProspero;
- x86-64 SysV cooperative context switching and guard-page stack handling;
- native multi-user DualSense lifecycle, borrowed handles and vibration;
- AudioOut integration and queue-lifetime handling;
- native AGC rendering research, shader preparation, tiling/descriptor experiments and VideoOut presentation;
- absolute-path/filesystem constraints for native PS5 titles;
- thread affinity/priority diagnostics;
- reproducible dependency pins for WiiCompiled, ps5link-sdk and SharpProspero;
- practical ShadowMount/kstuff/native-title deployment notes.

How to use it:

- treat the PS5 platform behavior and API observations as engineering evidence;
- independently implement reusable host functionality inside `ps5rt`;
- do **not** copy GPL implementation files into a differently licensed runtime without a deliberate licensing decision;
- do not confuse static game recompilation with Dolphin's dynamic PPC JIT/MMU architecture.

For Dolphin itself, this repo is therefore a **platform donor/reference**, while Dolphin upstream remains the emulator architecture.

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

Audit note (2026-10-01): public `ps5-payload-dev/sdk` remains available; the
latest checked revision is `f7fd02e6e195902a449b5e664917be8c01888b34`
(`add support for llvm-23`).

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
