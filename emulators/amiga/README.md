# Commodore Amiga

## Progress: **10%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

Preferred base: PUAE / libretro-uae lineage.

Reference: https://github.com/libretro/libretro-uae

Target machines include OCS, ECS and AGA families.

## Port direction

- preserve UAE CPU/chipset emulation;
- use shared PS5 filesystem/input/audio;
- map floppy/HDF/CD content cleanly;
- keep Kickstart firmware user-supplied.

## Next milestone

Standalone/static core boot with one A500 and one A1200 configuration.
