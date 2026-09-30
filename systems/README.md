# System Target Catalog

This directory is the canonical list of machines NATIVE-EMUS-PS5 intends to preserve as native PS5 emulator targets.

## M8 baseline — 43 targets

- [Nintendo Entertainment System](nes/README.md) — Mesen 2 / FCEUmm
- [Super Nintendo / Super Famicom](snes/README.md) — Snes9x / bsnes
- [Game Boy](gb/README.md) — SameBoy / Gambatte
- [Game Boy Color](gbc/README.md) — SameBoy / Gambatte
- [Game Boy Advance](gba/README.md) — mGBA
- [Virtual Boy](virtualboy/README.md) — Beetle VB / Mednafen
- [Nintendo 64](n64/README.md) — Mupen64Plus
- [Nintendo DS](nds/README.md) — DeSmuME / melonDS
- [Nintendo 3DS](3ds/README.md) — Azahar
- [Nintendo GameCube](gamecube/README.md) — Dolphin
- [Nintendo Wii](wii/README.md) — Dolphin
- [Nintendo Wii U](wiiu/README.md) — Cemu
- [Sega SG-1000](sg1000/README.md) — Genesis Plus GX
- [Sega Master System](mastersystem/README.md) — Genesis Plus GX
- [Sega Game Gear](gamegear/README.md) — Genesis Plus GX
- [Mega Drive / Genesis](megadrive/README.md) — Genesis Plus GX
- [Sega CD / Mega-CD](segacd/README.md) — Genesis Plus GX
- [Sega 32X](sega32x/README.md) — PicoDrive
- [Sega Saturn](saturn/README.md) — Beetle Saturn / Mednafen
- [Dreamcast / Naomi / Atomiswave](dreamcast/README.md) — Flycast
- [WonderSwan / WonderSwan Color](wonderswan/README.md) — Beetle WonderSwan / Mednafen
- [PlayStation](ps1/README.md) — Beetle PSX HW / DuckStation
- [PlayStation 2](ps2/README.md) — PCSX2 / PS5SX2
- [PlayStation 3](ps3/README.md) — RPCS3 / PS5_RPCS3
- [PlayStation Portable](psp/README.md) — PPSSPP
- [PlayStation Vita](vita/README.md) — Vita3K
- [Xbox](xbox/README.md) — xemu
- [Xbox 360](xbox360/README.md) — Xenia
- [Arcade / MAME](mame/README.md) — MAME
- [Neo Geo AES / MVS](neogeo/README.md) — FinalBurn Neo / MAME
- [Neo Geo Pocket / Color](ngp/README.md) — Beetle NeoPop
- [Atari 2600](atari2600/README.md) — Stella
- [Atari 5200](atari5200/README.md) — Atari800
- [Atari 7800](atari7800/README.md) — ProSystem
- [Atari Lynx](atarilynx/README.md) — Beetle Lynx / Handy
- [Atari Jaguar](atarijaguar/README.md) — Virtual Jaguar
- [Commodore 64](c64/README.md) — VICE
- [MSX / MSX2](msx/README.md) — blueMSX
- [ZX Spectrum](zxspectrum/README.md) — Fuse
- [ColecoVision](colecovision/README.md) — Gearcoleco / blueMSX
- [PC Engine / TurboGrafx-16](pcengine/README.md) — Beetle PCE Fast / Mednafen
- [PC Engine CD / TurboGrafx-CD](pcenginecd/README.md) — Beetle PCE / Mednafen
- [SuperGrafx](supergrafx/README.md) — Beetle SuperGrafx / Mednafen

## Additional systems discussed after / outside the M8 baseline

- [Nintendo Switch](switch/README.md) — Eden-derived / ProsperoEden reference
- [Neo Geo CD](neogeocd/README.md) — NeoCD / FinalBurn Neo
- [NEC PC-FX](pcfx/README.md) — Beetle PC-FX / Mednafen
- [Intellivision](intellivision/README.md) — FreeIntv
- [Commodore Amiga](amiga/README.md) — PUAE

## Meaning of a system target

A system target is not necessarily one separate emulator codebase. For example Dolphin covers GameCube and Wii, Genesis Plus GX covers several Sega machines, and Mednafen/Beetle cores cover multiple historical systems.

The rule remains: the final PS5 path should be native and direct, with an independent executable/package where that makes engineering and UX sense. Shared platform code belongs in `runtime/`, not in a chain of nested emulator frontends.
