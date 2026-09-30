# Atari ST / STE / TT / Falcon

Preferred base: Hatari.

Reference: https://github.com/hatari/hatari

## Port direction

Hatari is already a standalone emulator, so the goal is a thin PS5 platform layer rather than a libretro dependency.

Replace desktop SDL/window/audio/input pieces while preserving the 68000-family and machine emulation.

## Next milestone

Headless/native PS5 boot into TOS with disk-image loading and DualSense keyboard mappings.
