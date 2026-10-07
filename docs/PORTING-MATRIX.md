# Porting Matrix

Last updated: 2026-09-30

This matrix is a technical routing table. The "track" column groups similar engineering work; it is not a quality ranking.

| System | Core / emulator | CPU path | Graphics path | Track | Useful PS5 reference |
|---|---|---|---|---|---|
| NES | Mesen 2 / FCEUmm | interpreter | software | A portable | recovered NES M7 |
| SNES | Snes9x / bsnes | interpreter | software | A portable | — |
| GB | SameBoy / Gambatte | interpreter | software | A portable | — |
| GBC | SameBoy / Gambatte | interpreter | software | A portable | — |
| GBA | mGBA | interpreter | software | A portable | — |
| Virtual Boy | Beetle VB | interpreter | software | A portable | Mednafen patterns |
| N64 | Mupen64Plus | x86-64 dynarec | Vulkan-capable plugin path | B JIT | mihawk-99/PS5_Mupen64Plus |
| Nintendo DS | DeSmuME | JIT optional | software/OpenGL-derived paths | B JIT | mihawk-99/PS5_DeSmuME |
| Nintendo 3DS | Azahar | Dynarmic x86-64 | Vulkan | C modern | mihawk-99/PS5_Azahar + PS5_Dynarmic |
| GameCube | Dolphin | x86-64 JIT | Vulkan | C modern | PS5_RetroArch Dolphin work + Phi1ow/mkwii-ps5 platform research |
| Wii | Dolphin | x86-64 JIT | Vulkan | C modern | PS5_RetroArch Dolphin work + Phi1ow/mkwii-ps5 platform research |
| Wii U | Cemu | PPC recompiler | Vulkan | C modern | shared RADV/JIT runtime |
| SG-1000 | Genesis Plus GX | interpreter | software | A portable | — |
| Master System | Genesis Plus GX | interpreter | software | A portable | — |
| Game Gear | Genesis Plus GX | interpreter | software | A portable | — |
| Mega Drive | Genesis Plus GX | interpreter | software | A portable | — |
| Sega CD | Genesis Plus GX | interpreter | software | A portable + optical VFS | shared VFS |
| Sega 32X | PicoDrive | SH2 dynarec optional | software | B JIT | shared JIT runtime |
| Saturn | Beetle Saturn | interpreter/JIT-dependent upstream path | software/hybrid | B complex-retro | mihawk-99/PS5_BeetleSaturn |
| Dreamcast / Naomi / Atomiswave | Flycast | x86-64 SH4 dynarec | Vulkan | C modern | shared RADV/JIT runtime |
| WonderSwan / Color | Beetle WonderSwan | interpreter | software | A portable | Mednafen patterns |
| PlayStation | Beetle PSX HW | **Lightrec + GNU Lightning x86-64** | Vulkan / RADV | B JIT | PS5_BeetlePSX donor + ps5rt executable pool |
| PlayStation 2 | PCSX2 | x86-64 recompilers | Vulkan | C modern | Swordpdf/PS5SX2 |
| PlayStation 3 | RPCS3 | PPU/SPU LLVM | Vulkan/RADV | C modern | mihawk-99/PS5_RPCS3 + PS5_LLVM + PS5_Mesa |
| PSP | PPSSPP | x86-64 JIT | Vulkan | B JIT | OpenAGC/ps5-ppsspp + PS5_RetroArch |
| PS Vita | Vita3K | Dynarmic x86-64 | Vulkan | C modern | mihawk-99/PS5_Dynarmic |
| Xbox | xemu | QEMU/TCG-derived | Vulkan | D machine-model | xemu upstream + PS5 RADV |
| Xbox 360 | Xenia | x64 backend | Vulkan | D GPU-heavy | Xenia upstream + PS5 RADV |
| MAME | MAME | many interpreters/dynarecs | software/GPU-dependent | D broad | mihawk-99/PS5_MAME |
| Neo Geo AES/MVS | FinalBurn Neo / MAME | interpreter | software | A portable | PS5_MAME donor work |
| Neo Geo Pocket/Color | Beetle NeoPop | interpreter | software | A portable | — |
| Atari 2600 | Stella | interpreter | software | A portable | — |
| Atari 5200 | Atari800 | interpreter | software | A portable | — |
| Atari 7800 | ProSystem | interpreter | software | A portable | — |
| Atari Lynx | Beetle Lynx | interpreter | software | A portable | Mednafen patterns |
| Atari Jaguar | Virtual Jaguar | 68k + RISC emulation | software | B complex-retro | — |
| Commodore 64 | VICE | interpreter | software | A portable | mihawk-99/PS5_VICE |
| MSX / MSX2 | blueMSX | interpreter | software | A portable | — |
| ZX Spectrum | Fuse | interpreter | software | A portable | — |
| ColecoVision | Gearcoleco / blueMSX | interpreter | software | A portable | — |
| PC Engine / TG16 | Beetle PCE Fast | interpreter | software | A portable | Mednafen patterns |
| PC Engine CD / TG-CD | Beetle PCE | interpreter | software | A portable + optical VFS | shared VFS |
| SuperGrafx | Beetle SuperGrafx | interpreter | software | A portable | Mednafen patterns |
| Nintendo Switch | Eden-derived | Dynarmic x86-64 | Vulkan/RADV | C modern | blackbearreloaded/ProsperoEden |
| Neo Geo CD | NeoCD / FBNeo | interpreter | software | A portable + optical VFS | shared VFS |
| NEC PC-FX | Beetle PC-FX | interpreter | software | B complex-retro | Mednafen patterns |
| Intellivision | FreeIntv | interpreter | software | A portable | — |
| Commodore Amiga | PUAE | 68k emulation/JIT depending build | software | B complex-retro | shared runtime |
| 3DO | Opera | ARM60 emulation | software | B complex-retro + optical VFS | ps5rt::media / VFS |
| Atari ST / STE / TT / Falcon | Hatari | 68k emulation | software | B complex-retro | keyboard/mouse + VFS |
| DOS / IBM PC | DOSBox Staging | interpreter/dynamic x86 core | software | B JIT + desktop I/O | keyboard/mouse + JIT |
| Philips CD-i | SAME CDi / MAME | 68000-family emulation | software | B complex-retro + optical VFS | ps5rt::media |
| Odyssey2 / Videopac | O2EM | interpreter | software | A portable | keyboard/keypad input |
| CPS-1 | FinalBurn Neo / MAME | interpreter | software | A arcade | shared arcade engine |
| CPS-2 | FinalBurn Neo / MAME | interpreter | software | A arcade | shared arcade engine |
| CPS-3 | FinalBurn Neo / MAME | interpreter | software | A arcade | shared arcade engine |
| Sega Naomi | Flycast | x86-64 SH4 dynarec | Vulkan | C modern arcade | Flycast + PS5 RADV |
| Sammy Atomiswave | Flycast | x86-64 SH4 dynarec | Vulkan | C modern arcade | Flycast + PS5 RADV |
| Sega Model 2 | MAME research | i960 + device emulation | software/custom 3D | D machine-model | MAME driver research |
| Sega Model 3 | Supermodel | PowerPC emulation | OpenGL today; PS5 renderer work needed | D GPU-heavy | standalone native port research |

## Track A — portable cores

These are useful for hardening the common PS5 host layer with relatively little CPU-backend risk:

- AudioOut latency and buffering
- DualSense mapping
- VFS and saves
- content browser
- packaging
- suspend/resume
- WebUI/settings

## Track B — JIT / complex retro

These additionally exercise:

- `ps5rt::jit`
- executable-memory sizing
- cache flushing
- thread stack sizing
- fault/exception behavior

## Track C — modern Vulkan/JIT

These are the strongest consumers of the scene work from Swordpdf, Mihawk and BlackBear:

- Zen 2 tuning
- Dynarmic / LLVM / x86-64 recompilers
- PS5 Mesa/RADV
- shader caches
- Vulkan submission/readback performance
- TLS and threading

## Track D — broad machine/GPU models

xemu, Xenia and MAME need more host/platform decomposition before they can share the same clean bring-up path as smaller cores.

## Cross-cutting rule

A solution discovered in one target moves into `runtime/` only when it is genuinely a PS5 host concern. Emulator-specific timing, guest memory semantics, GPU translation and device emulation stay in the emulator.
