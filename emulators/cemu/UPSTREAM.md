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

## PS5 scene reference: PS5CEMU

Reference repository: https://github.com/premohq/PS5CEMU

Audited revision:

```text
65de61fa2843d69b384fede0205189ea0b54001a
```

Use this as a PS5-specific donor/reference, not as the canonical Cemu source
base. At the audited revision it contains a complete PS5 app build pipeline,
headless/launcher integration, native Cemu platform classes, RADV/Vulkan,
DualSense, AudioOut, JIT-memory handling and packaging work.

Important boundaries:

- upstream Cemu remains `cemu-project/Cemu`;
- PS5CEMU pins a different Cemu revision
  (`4e3c824faa00f6b85782db019f20f29f063f3a2a`), so do not transplant patches
  blindly onto our newer Cemu pin;
- PS5CEMU-owned code is GPL-3.0-or-later while Cemu-derived files retain
  MPL-2.0; copy/adapt only after per-file license review;
- the audited README explicitly describes the app as built but not yet run on
  physical PS5 hardware, so it is engineering evidence rather than a validated
  compatibility claim.
