# Flycast PS5 — rec-x64 engine gate

This port directory owns the **native PS5 cross-build gate** for Flycast.

The first gate deliberately builds the emulator as an engine-only static archive. It does not replace the emulator with an interpreter and it does not wrap a Linux executable.

## CPU path

```text
SH4 guest
  -> Flycast dynarec
  -> rec-x64 / Xbyak
  -> ps5rt JIT/fastmem adapter
  -> PS5 Zen 2
```

The build fails if the resulting archive does not contain the upstream `rec_x64.cpp` object.

## Why engine-only first

Flycast's desktop standalone target mixes the core with SDL, X11, desktop audio, DreamLink/libusb and other host services. Those are intentionally excluded from this gate.

This gives us a clean boundary:

1. compile the real emulator + rec-x64 for PS5;
2. validate PS5 fastmem/JIT;
3. attach native AudioOut/input/storage;
4. enable the upstream Vulkan renderer against PS5 Mesa/RADV;
5. add the PS5 application shell.

## Pinned upstream

`flyinghead/flycast@e36e9df2dcc1487acdb1dc7725766f1f5ba029b5`

## Current renderer policy

The engine-only CI gate temporarily configures Vulkan OFF so CPU/JIT/platform compilation failures are isolated from Vulkan dependencies.

The final port remains **Vulkan/RADV**, not software-only and not an OpenGL wrapper.
