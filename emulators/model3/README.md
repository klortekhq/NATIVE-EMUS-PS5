# Sega Model 3

## Progress: **10%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

Preferred base: Supermodel.

Reference: https://github.com/trzy/Supermodel

## Port direction

Supermodel is standalone and open source. The largest porting tasks are:

- PowerPC host CPU execution path;
- OpenGL renderer replacement/adaptation;
- SDL2 host services;
- input, force feedback and network features.

A future Vulkan renderer or a PS5-native graphics adaptation is preferable to carrying a desktop OpenGL stack.

## Next milestone

Compile the core machine/CPU code with the PS5 toolchain and isolate renderer dependencies.
