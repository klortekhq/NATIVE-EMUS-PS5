# NATIVE SNES — Snes9x

Standalone native PS5 Snes9x port using the shared `ps5rt` host.

## Upstream pin

- `libretro/snes9x`
- commit `fae2fea08f74180759ef540ee94259213f503480`
- codeload SHA-256 `0d4b0c4181d66668ec0040eb17f90a559f7b7fa6cbd9198593c3bdbe79acd029`

The core is statically linked into the native title. RetroArch is not a runtime dependency.

## Native path

`Snes9x -> static core ABI -> ps5rt -> VideoOut / AudioOut / DualSense / VFS`

The PS5 audio backend accepts the core sample rate and converts it to the hardware 48 kHz stream while preserving rate across callback batches.

## Content

Default: `/data/NATIVE-EMUS-PS5/SNES/game.sfc`.

`rom-path.txt` in the same directory may point to any readable `.sfc`/`.smc` file. USB fallback follows the common host policy.

## Build

```bash
bash ports/snes9x-ps5/build_ps5.sh
```

Status is **hardware-validation pending** until a generated PS5 binary is tested on console.
