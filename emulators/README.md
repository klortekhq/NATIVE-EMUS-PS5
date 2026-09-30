# Emulator Workspaces

These directories track the concrete upstream cores/codebases being adapted or evaluated for native PS5 use.

- [atari800](atari800/README.md)
- [azahar](azahar/README.md)
- [beetle-neopop](beetle-neopop/README.md)
- [beetle-psx](beetle-psx/README.md)
- [beetle-saturn](beetle-saturn/README.md)
- [bluemsx](bluemsx/README.md)
- [cemu](cemu/README.md)
- [desmume](desmume/README.md)
- [dolphin](dolphin/README.md)
- [finalburn-neo](finalburn-neo/README.md)
- [flycast](flycast/README.md)
- [freeintv](freeintv/README.md)
- [fuse](fuse/README.md)
- [gearcoleco](gearcoleco/README.md)
- [genesis-plus-gx](genesis-plus-gx/README.md)
- [mame](mame/README.md)
- [mednafen](mednafen/README.md)
- [mesen2](mesen2/README.md)
- [mgba](mgba/README.md)
- [mupen64plus](mupen64plus/README.md)
- [neocd](neocd/README.md)
- [pcsx2](pcsx2/README.md)
- [picodrive](picodrive/README.md)
- [ppsspp](ppsspp/README.md)
- [prosystem](prosystem/README.md)
- [puae](puae/README.md)
- [rpcs3](rpcs3/README.md)
- [sameboy](sameboy/README.md)
- [snes9x](snes9x/README.md)
- [stella](stella/README.md)
- [switch](switch/README.md)
- [vice](vice/README.md)
- [virtualjaguar](virtualjaguar/README.md)
- [vita3k](vita3k/README.md)
- [xemu](xemu/README.md)
- [xenia](xenia/README.md)

## Systems versus emulators

The canonical machine list lives in [`systems/`](../systems/README.md).

One emulator can serve multiple systems, and one system can have more than one candidate core. Keeping these two indexes separate prevents the project from inventing duplicate ports just to satisfy a one-folder-per-console layout.

## Rule

An emulator workspace is allowed to depend on `runtime/`, its own upstream dependencies and legally compatible third-party libraries. It should not depend on another emulator frontend as a mandatory execution layer.
