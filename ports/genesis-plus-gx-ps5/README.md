# NATIVE SEGA 8/16-bit — Genesis Plus GX

One statically linked Genesis Plus GX engine produces native PS5 builds for:

- SG-1000
- Master System
- Game Gear
- Mega Drive / Genesis

Sega CD / Mega-CD is intentionally tracked as the next gate because it adds optical-media and disk-control requirements.

## Upstream pin

- `libretro/Genesis-Plus-GX`
- commit `c2838c7dc4236fc2fe94e5dbd08b41486067918e`
- codeload SHA-256 `7ba2eab9d6dae71bb42e8208573300ad3475a4263e860178c4d19c36d85fc92b`

## Architecture

`Genesis Plus GX (static) -> native_libretro_host -> ps5rt -> PS5 native services`

The core outputs 44.1 kHz audio; `ps5rt::AudioDevice` converts it statefully to PS5 AudioOut 48 kHz without changing the core.

## Build

```bash
bash ports/genesis-plus-gx-ps5/build_ps5.sh
```

The default build emits all four cartridge-family PIE ELFs. No RetroArch runtime is involved.

## Content defaults

- `SG1000/game.sg`
- `MASTERSYSTEM/game.sms`
- `GAMEGEAR/game.gg`
- `MEGADRIVE/game.md`

Each system also accepts `rom-path.txt` and USB fallback through the common host.

## Status gate

Cross-build success means **PS5 binary produced**. Playability is only claimed after hardware testing on a jailbroken PS5.
