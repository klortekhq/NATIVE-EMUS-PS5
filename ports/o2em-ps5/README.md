# NATIVE Odyssey2 / Videopac — O2EM

Standalone native PS5 application using the pinned O2EM libretro core and the shared `corehost + ps5rt` runtime.

## Runtime

- native PS5 executable
- no RetroArch process
- statically linked core
- VideoOut
- AudioOut
- DualSense
- keyboard surface available through corehost
- SRAM/save surface when exposed by the core
- clean exit: **Create + Options**

## Content

Default:

`/data/NATIVE-EMUS-PS5/ODYSSEY2/game.bin`

Or put an absolute game path in:

`/data/NATIVE-EMUS-PS5/ODYSSEY2/boot.txt`

## Firmware

Required user-supplied BIOS:

`/data/NATIVE-EMUS-PS5/ODYSSEY2/system/o2rom.bin`

Upstream can also select compatible regional Videopac+ BIOS files through its core option.

## Pin

`libretro/libretro-o2em@679d6fec04963f6e70a7ec217e3d0ebb1fe472fc`
