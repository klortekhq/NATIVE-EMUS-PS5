# Cemu / Wii U

## Progress: **25%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

## Desired architecture

Cemu already provides the important pieces:

- PPC emulation/recompiler
- Vulkan renderer
- cross-platform core

The main problem is removing desktop/UI assumptions cleanly.

## Porting focus

- identify wxWidgets boundary
- establish a headless boot path
- replace host input/audio/window services
- adapt executable memory
- route Vulkan to PS5 Mesa/RADV
- map Wii U GamePad concepts to PS5 UX without contaminating the core

## Avoid

Do not port the desktop GUI first.

The first PS5 build should boot a title from a fixed path with logs before any full frontend exists.

## Next milestone

Document the minimal Cemu target/source set required to initialize the emulator core and Vulkan renderer without wxWidgets UI.
