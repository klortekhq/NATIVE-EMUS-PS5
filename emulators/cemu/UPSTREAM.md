# Cemu upstream pin

Canonical repository: https://github.com/cemu-project/Cemu

## Current project pin

```text
c717fcab1ccc3e0b0b97499a4d9b04a77084e347
```

Observed as current upstream head during the 2026-09-30 audit.

## License

`LICENSE.txt` contains the Mozilla Public License 2.0 text.

Third-party dependencies retain their own licenses and require inventory before redistribution.

## Integration policy

Use canonical Cemu as the source base. PS5-specific work should sit behind existing platform abstractions (MemMapper, WindowSystem, IAudioAPI, ControllerProvider, CafeSystem::SystemImplementation) instead of copying desktop frontend code.
