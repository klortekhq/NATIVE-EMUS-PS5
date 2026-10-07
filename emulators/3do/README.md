# 3DO Interactive Multiplayer

## Progress: **10%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

Preferred base: Opera.

Upstream reference: https://github.com/libretro/opera-libretro

## Port direction

- keep the 3DO machine core;
- remove dynamic frontend dependency;
- static/native PS5 integration;
- use shared VFS/audio/input;
- validate BIOS handling without distributing firmware.

## Next milestone

Compile the core as a PS5 static engine and boot firmware with logging and controller input.
