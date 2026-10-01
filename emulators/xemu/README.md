# xemu / Original Xbox

## Progress: **15%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

## Challenge

xemu is deeper than most targets because it inherits a substantial QEMU-derived machine model.

That is legitimate emulator architecture, but it means more host/platform dependencies must be isolated.

## Desired direction

- keep Xbox machine/device model
- keep CPU translation architecture
- keep Vulkan renderer where possible
- remove desktop UI/audio/input/window assumptions
- provide PS5 host services directly

## Special investigation

Track upstream RDNA2 performance behavior carefully because PS5 is RDNA2-class hardware.

## Next milestone

Build a dependency graph separating:

1. Xbox machine emulation
2. QEMU infrastructure actually required
3. desktop-only infrastructure
4. renderer
5. host I/O

Then define the smallest PS5-hostable target.

## PS5 scene reference: XPSemu

Reference repository: https://github.com/ZiZc3/XPSemu

Audited revision:

```text
eca27ad0633c19ca55ba48e5d036f068b6295ba0
```

This is a concrete PS5 xemu donor/reference. At the audited revision its
README describes a native PS5 app using xemu/QEMU TCG, Vulkan/RADV, native
DualSense input, AudioOut, PS5 memory/JIT adaptation and a PS5 dashboard.

### License/provenance boundary

GitHub reports the repository license as `NOASSERTION`. The checked tree does
contain the QEMU/xemu top-level GPL-2.0 license material plus a large set of
third-party license files, but that is not enough to treat the aggregate tree
as a single-license donor.

Therefore:

- preserve the repository/revision as **metadata only** for now;
- do not vendor or snapshot XPSemu source into this repository yet;
- review the exact file license and third-party provenance before copying or
  adapting implementation code;
- keep canonical `xemu-project/xemu` as the source base.

### Hardware evidence

The audited XPSemu README says its Alpha 1 build was tested by its developer on
a physical PS5 and includes several game-specific observations. That is useful
external feasibility evidence, but it is **not** NATIVE-EMUS-PS5 hardware
validation. Our own xemu/PS5 claims remain gated on reproducible builds and
local physical-console testing.
