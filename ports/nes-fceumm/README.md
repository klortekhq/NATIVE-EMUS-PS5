# NATIVE NES — FCEUmm

First full emulator port built on the shared native PS5 host.

## Architecture

```text
FCEUmm (static) -> native_libretro_host -> ps5rt
                                      -> VideoOut
                                      -> AudioOut
                                      -> DualSense
                                      -> PS5 VFS / SRAM
```

There is **no RetroArch executable or runtime dependency**. Libretro is only the statically linked core ABI.

## Pinned upstream

- `libretro/libretro-fceumm`
- commit `236ccdfc911e84c60fea6b9d0699c2d440a8de14`
- codeload SHA-256 `dd002cde9b5271979e0394bb9e696bd37e149ced473ff1e3629cc7fed502381f`

This is intentionally the M8-known revision for first hardware validation. Updating the core and changing the PS5 host at the same time would make failures harder to isolate.

## Content search

1. `/data/NATIVE-EMUS-PS5/NES/rom-path.txt`
2. `/app0/rom-path.txt`
3. `/data/NATIVE-EMUS-PS5/NES/game.nes`
4. `/mnt/usb0/NATIVE-EMUS-PS5/NES/game.nes`
5. `/mnt/usb1/NATIVE-EMUS-PS5/NES/game.nes`
6. `/app0/game.nes`

No ROM is distributed.

## Controls

- D-pad -> NES D-pad
- Cross -> A
- Square -> B
- Options -> Start
- Touchpad click -> Select
- Options + Touchpad click -> exit

## Build

```bash
./ports/nes-fceumm/build_ps5.sh
```

Required: `PS5_PAYLOAD_SDK` with `prospero-clang`, `prospero-clang++`, `prospero-ar` and the public PS5 stubs.

Optional: `PS5_NATIVE_TOOL`; when supplied, the script also converts the linked PIE to native-title ELF/FSELF.

## Status gate

The integration/build path may be marked **build-ready** after CI/static checks. It is only marked **playable** after the resulting binary runs on PS5 hardware and video, audio, input and SRAM are validated.
