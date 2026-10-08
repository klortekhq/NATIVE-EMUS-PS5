# NATIVE-EMUS-PS5

Native emulator ports and shared runtime research for jailbroken PlayStation 5 systems.

> **Goal:** run emulator cores directly on PS5 hardware, using native x86-64 recompilers/JITs where available and native PS5 graphics/audio/input APIs.  
> **Rule:** no Linux/Wine/VM wrapper stack as the project architecture, and no "Frankenstein" chain of emulators inside emulators.

## Project principles

- **Native first.** Emulator CPU backends should target the PS5's x86-64 Zen 2 CPU directly when upstream supports it.
- **Keep upstream cores recognizable.** Port the platform layer around the emulator instead of rewriting the emulator unnecessarily.
- **Shared PS5 runtime.** Memory/JIT, threading, TLS, Vulkan/RADV, audio, input, filesystem and crash handling should be solved once and reused.
- **Vulkan/RADV first.** Mesa/RADV work in the PS5 scene has reached the point where it should be the common graphics path for modern cores whenever practical.
- **Clean separation.** Every emulator remains a standalone port with its own upstream, license and compatibility story.
- **No proprietary content.** No firmware, BIOS, keys, games or Sony proprietary SDK material belongs in this repository.
- **Evidence over hype.** A port is only marked working when code/build/runtime evidence exists.

## Complete target coverage

The recovered M8 baseline contained **43 system targets**. The project has now been expanded to **60 distinct systems / hardware families**, including every additional platform explicitly discussed: Switch, Neo Geo CD, PC-FX, 3DO, Amiga, Atari ST, DOS, CD-i, Intellivision, Odyssey² and board-specific arcade targets such as CPS-1/2/3, Naomi, Atomiswave, Model 2 and Model 3.

Every target is represented in [`systems/targets.json`](systems/targets.json) and documented through [`systems/`](systems/README.md).

Concrete codebase workspaces live under [`emulators/`](emulators/README.md), while the shared PS5 host contract lives under [`runtime/`](runtime/README.md).

See the [complete 60-target system matrix](docs/COMPLETE-SYSTEM-MATRIX.md) and the [porting matrix](docs/PORTING-MATRIX.md) for CPU/JIT/GPU requirements and current PS5 references.

## First native cross-builds

The first repository-native PS5 ports now complete the full public-SDK cross-build pipeline:

- **NES — FCEUmm**
- **SNES — Snes9x**
- **SG-1000 — Genesis Plus GX**
- **Master System — Genesis Plus GX**
- **Game Gear — Genesis Plus GX**
- **Mega Drive / Genesis — Genesis Plus GX**

These builds statically link their emulator core into a PS5-targeted ELF and use the shared native `ps5rt` VideoOut, AudioOut, DualSense and storage host. They do **not** require a RetroArch executable or Linux/Wine layer.

Status: **cross-built; physical PS5 validation pending**.

## Current focus

| Platform | Native CPU path | Graphics path | Current project status |
|---|---|---|---|
| PlayStation 2 | x86-64 recompiler | Vulkan | Reference implementation / integration research |
| Dreamcast / Naomi / Atomiswave | x86-64 dynarec | Vulkan | High-priority port target |
| PlayStation 3 | PPU/SPU LLVM | Vulkan / RADV | Active scene breakthrough; high-priority research |
| PlayStation Vita | Dynarmic x86-64 | Vulkan | High-priority port target |
| Wii U | PPC recompiler | Vulkan | Port architecture research |
| Xbox | QEMU/TCG-derived | Vulkan | Platform research |
| Xbox 360 | x64 backend | Vulkan | Platform/GPU research |
| Nintendo Switch | Dynarmic x86-64 | Vulkan / RADV | Reference implementation research |
| PSP | x86-64 JIT | Vulkan | Existing PS5 port research |
| Nintendo 64 | x86-64 dynarec | Vulkan-capable plugins | Existing PS5 port research |
| Nintendo 3DS | Dynarmic x86-64 | Vulkan | Existing PS5 scene work |
| GameCube / Wii | Jit64 + x64 DSP JIT | Vulkan | PS5 engine scaffold cross-built; Vulkan compile/link/presentation gates remain |

## Shared runtime target

```text
emu core
  |
  +-- CPU/JIT --------> ps5rt::jit / executable memory / cache flush
  +-- GPU ------------> ps5rt::vulkan -> Mesa/RADV -> PS5 GPU
  +-- threads/TLS ----> ps5rt::thread / ps5rt::tls
  +-- audio ----------> ps5rt::audio -> libSceAudioOut
  +-- input ----------> ps5rt::input -> DualSense / keyboard / mouse
  +-- filesystem -----> ps5rt::vfs -> /data / USB / M.2 / optional network paths
  +-- lifecycle ------> ps5rt::app / logging / crash handling
  +-- packaging ------> native PS5 title tooling
```

The intention is **not** to force every emulator through one frontend. The shared layer exists to remove duplicated PS5 platform work while keeping each emulator native and independently maintainable.

## Repository layout

```text
systems/       canonical 60-target machine/console/arcade hardware catalog
emulators/     concrete upstream emulator/core workspaces
runtime/       shared native PS5 host contract (JIT, memory, TLS, audio, input, VFS, Vulkan)
docs/          architecture, status, roadmap, scene references and porting matrix
legacy/        recovered historical M0→M8 material and patches
artifacts/     hashes/manifests for recovered build artifacts
```

Emulator source is imported only when there is a clean, license-compatible integration strategy. Historical build packs stay separate from the current source layout.

## Status language

- **Research** — architecture/source review only.
- **Scaffold** — PS5 build/platform skeleton exists.
- **Cross-built** — the emulator core and PS5 host compile/link into a PS5-targeted binary in reproducible CI; physical console validation is still pending.
- **Booting** — emulator reaches its own startup/BIOS/UI on PS5.
- **In-game** — at least one title reaches gameplay.
- **Playable** — sustained play with working input/audio/rendering.
- **Validated** — repeatable builds and broader compatibility testing.

## Legal / licensing

This repository is intended for lawful homebrew and interoperability research. Upstream emulator licenses remain in force. No game images, console BIOS/firmware, cryptographic keys or proprietary Sony SDK files are distributed here.

---

**Klortek · NATIVE-EMUS-PS5**
