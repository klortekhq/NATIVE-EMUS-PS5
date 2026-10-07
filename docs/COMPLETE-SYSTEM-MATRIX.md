# Complete System Matrix

Last reviewed: 2026-09-30

This is the canonical **coverage list** for NATIVE-EMUS-PS5.

A system being listed here means the project intends to track it. It does **not** mean that a native PS5 build already exists.

## Coverage rule

One emulator engine may cover multiple systems:

- Flycast -> Dreamcast, Naomi, Atomiswave
- Dolphin -> GameCube, Wii
- Genesis Plus GX -> SG-1000, Master System, Game Gear, Mega Drive, Sega CD
- FBNeo -> Neo Geo, CPS1/2/3 and many arcade boards
- mGBA -> GB, GBC, GBA
- MAME -> broad arcade/microcomputer fallback

## Full catalog

| Family | System / hardware | Preferred engine/base | PS5 plan |
|---|---|---|---|
| Arcade | MAME/general arcade | MAME | native/static PS5 build; per-driver validation |
| Arcade | Neo Geo AES/MVS | FinalBurn Neo | standalone/static engine |
| Arcade | Neo Geo CD | NeoCD / FBNeo | dedicated CD path |
| Arcade | Capcom CPS-1 | FinalBurn Neo / MAME | shared arcade engine |
| Arcade | Capcom CPS-2 | FinalBurn Neo / MAME | shared arcade engine |
| Arcade | Capcom CPS-3 | FinalBurn Neo / MAME | shared arcade engine |
| Arcade | Sega Naomi | Flycast | shared Flycast engine |
| Arcade | Sega Atomiswave | Flycast | shared Flycast engine |
| Arcade | Sega Model 2 | MAME research / Model 2 reference | research; no Windows wrapper architecture |
| Arcade | Sega Model 3 | Supermodel | native port candidate |
| Atari | Atari 2600 | Stella | static/native port |
| Atari | Atari 5200 | Atari800 | static/native port |
| Atari | Atari 7800 | ProSystem | static/native port |
| Atari | Atari Lynx | Beetle Lynx | static/native port |
| Atari | Atari Jaguar | Virtual Jaguar | static/native port |
| Atari | Atari ST / STE / TT / Falcon | Hatari | standalone/native port |
| Commodore | Commodore 64 | VICE x64sc | existing PS5 donor knowledge |
| Commodore | Amiga OCS/ECS/AGA | PUAE | static/native engine |
| PC | DOS / PC compatibles | DOSBox Staging | standalone native port |
| Microsoft | Xbox | xemu | major native port |
| Microsoft | Xbox 360 | Xenia | major native port |
| NEC | PC Engine / TurboGrafx-16 | Beetle PCE Fast | static/native port |
| NEC | PC Engine CD / TurboGrafx-CD | Beetle PCE Fast | shared PCE engine |
| NEC | SuperGrafx | Beetle SuperGrafx | static/native port |
| NEC | PC-FX | Beetle PC-FX | static/native port |
| Nintendo | NES | FCEUmm | PS5-proven donor / native target |
| Nintendo | SNES | Snes9x | PS5-proven donor / native target |
| Nintendo | Game Boy | mGBA | shared native engine |
| Nintendo | Game Boy Color | mGBA | shared native engine |
| Nintendo | Game Boy Advance | mGBA | shared native engine |
| Nintendo | Virtual Boy | Beetle VB | static/native port |
| Nintendo | Nintendo 64 | Mupen64Plus | x86-64 dynarec + Vulkan |
| Nintendo | Nintendo DS | DeSmuME | x86 JIT |
| Nintendo | Nintendo 3DS | Azahar | Dynarmic x86-64 + Vulkan |
| Nintendo | GameCube | Dolphin | Jit64 + Vulkan |
| Nintendo | Wii | Dolphin | Jit64 + Vulkan |
| Nintendo | Wii U | Cemu | x64 PPC recompiler + Vulkan |
| Nintendo | Switch | ProsperoEden reference / compatible upstream research | Dynarmic x86-64 + RADV |
| Sega | SG-1000 | Genesis Plus GX | shared Sega 8/16-bit engine |
| Sega | Master System | Genesis Plus GX | shared Sega 8/16-bit engine |
| Sega | Game Gear | Genesis Plus GX | shared Sega 8/16-bit engine |
| Sega | Mega Drive / Genesis | Genesis Plus GX | PS5-proven donor |
| Sega | Sega CD / Mega-CD | Genesis Plus GX | shared CD path |
| Sega | 32X | PicoDrive | static/native port |
| Sega | Saturn | Kronos/YabaSanshiro candidate + Beetle donor | native SH2 dynarec preferred |
| Sega | Dreamcast | Flycast | x64 SH4 dynarec + Vulkan |
| SNK | Neo Geo Pocket / Color | Beetle NeoPop | static/native port |
| Bandai | WonderSwan / Color | Beetle WonderSwan | static/native port |
| Mattel | Intellivision | FreeIntv | static/native port |
| Magnavox/Philips | Odyssey2 / Videopac G7000 | O2EM | static/native port |
| Philips | CD-i | SAME CDi / MAME reference | static/native research |
| Coleco | ColecoVision | blueMSX | shared 8-bit engine |
| Microsoft/ASCII | MSX / MSX2 | blueMSX | shared 8-bit engine |
| Sinclair | ZX Spectrum | Fuse | static/native port |
| Panasonic/3DO | 3DO Interactive Multiplayer | Opera | static/native port |
| Sony | PlayStation | SwanStation/DuckStation-lineage | x86-64 recompiler + Vulkan |
| Sony | PlayStation 2 | PCSX2 | native x86-64 recompilers + Vulkan |
| Sony | PlayStation 3 | RPCS3 | LLVM PPU/SPU + RADV |
| Sony | PSP | PPSSPP | x86-64 JIT + Vulkan |
| Sony | PlayStation Vita | Vita3K | Dynarmic + Vulkan |

## Not emulator targets

- **PS4 on PS5**: handled by Sony's native backward-compatibility environment, not a guest emulator project target here.
- **PS5**: native host platform, not an emulated target.

Those platforms can still have launcher/library tooling, but they are not placed in the emulator-core roadmap.

## Count

The current catalog contains **60 distinct systems / hardware families** when major arcade boards are counted separately.

This supersedes the old M8 list of 43 targets as the project coverage baseline.
