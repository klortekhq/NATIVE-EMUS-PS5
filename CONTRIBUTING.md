# Contributing

Contributions are welcome when they keep the project technically clean and legally redistributable.

## Before opening a pull request

Please make sure the change follows these rules:

- keep emulator upstream code recognizable;
- isolate PS5-specific platform work;
- do not add proprietary Sony SDK material;
- do not add BIOS, firmware, game images or cryptographic keys;
- document the upstream revision used;
- preserve upstream licenses and attribution;
- prefer native x86-64/JIT/Vulkan paths over compatibility-layer stacks;
- do not mark a target as working without reproducible evidence.

## Commit scope

Prefer focused commits:

- `runtime:` shared PS5 platform work
- `flycast:` Dreamcast/Naomi/Atomiswave
- `rpcs3:` PlayStation 3
- `vita3k:` PlayStation Vita
- `cemu:` Wii U
- `xemu:` Xbox
- `xenia:` Xbox 360
- `docs:` architecture/research/status

## Port evidence

When reporting a milestone, include as much as possible:

- PS5 model
- firmware
- upstream commit
- toolchain/SDK commit
- renderer
- JIT/interpreter mode
- logs
- reproducible build command
- screenshot/video only as supporting evidence, not instead of logs

## Upstream first

If a fix is not PS5-specific and can reasonably be upstreamed to the emulator project, prefer doing that rather than carrying a permanent fork-only patch.
