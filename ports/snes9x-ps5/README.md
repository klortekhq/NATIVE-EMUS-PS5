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


## Verified native-title evidence

Workflow `36858894742` completed the full public native-title pipeline:

1. cross-build Snes9x + shared `corehost/ps5rt` into the intermediate PIE;
2. verify the dynamic symbol table contains imports only;
3. convert the PIE with the pinned BlackBear native-title tooling;
4. produce and inspect `eboot.elf` + signed `eboot.bin`;
5. upload the hashes and final native files.

Artifact:

- name: `native-snes-snes9x-ps5-fself`
- artifact ID: `11161130046`
- artifact ZIP SHA-256:
  `62b139f01a772649d8749e122c786cbd4faef290c7dd4aed1f1325822e97e4df`

The successful finalization uses the verified BlackBear page-separated PIE
layout plus the exact linker-visible boundary symbols required by the public
`ps5-payload-dev/sdk` linker contract (image/text, dynamic, EH-frame and BSS
bounds).

This is reproducible native-title evidence, not physical-console boot evidence;
hardware validation remains pending.
