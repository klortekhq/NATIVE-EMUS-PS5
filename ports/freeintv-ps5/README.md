# NATIVE Intellivision — FreeIntv

Standalone native PS5 application using the pinned FreeIntv core and the shared `corehost + ps5rt` runtime.

## CPU policy

FreeIntv's Intellivision CPU emulation is kept exactly as provided by upstream. This small historical machine does not gain anything from inventing a fake JIT layer.

## Runtime

- native PS5 executable
- no RetroArch process
- statically linked core
- VideoOut software-frame path
- AudioOut with automatic source-rate conversion to PS5 48 kHz
- DualSense
- SRAM persistence
- core options v0
- clean exit: **Create + Options**

## Content

Default:

`/data/NATIVE-EMUS-PS5/INTELLIVISION/game.int`

Or place an absolute game path in:

`/data/NATIVE-EMUS-PS5/INTELLIVISION/boot.txt`

Supported upstream extensions: `.int`, `.bin`, `.rom`.

## Firmware

User-supplied files required in:

`/data/NATIVE-EMUS-PS5/INTELLIVISION/system/`

- `exec.bin`
- `grom.bin`

Firmware is not distributed by this repository.

## Pin

`libretro/FreeIntv@ef3e0fe322bec62a7f916c0bb0834c08c348d0b4`
