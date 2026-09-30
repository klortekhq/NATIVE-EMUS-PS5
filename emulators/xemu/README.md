# xemu / Original Xbox

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
