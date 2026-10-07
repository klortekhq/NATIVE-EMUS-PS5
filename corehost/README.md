# corehost

A small **static core host** for native PS5 emulator applications.

This is deliberately **not RetroArch**. It implements only the frontend-side ABI needed to link suitable libretro-compatible emulator cores directly into an independent PS5 executable.

## Architecture

```text
emulator core (statically linked)
        |
        v
corehost
        |
        v
ps5rt
        |
        +--> AudioOut
        +--> DualSense / keyboard / mouse
        +--> VFS / optical media / SMB
        +--> framebuffer / Vulkan where appropriate
```

There is no dynamic core loader and no RetroArch process/frontend.

## Current surface

- lifecycle/init/deinit
- content-by-path loading
- software video callback + pixel format
- stereo s16 audio callbacks
- joypad bitmask
- analog sticks/triggers
- USB HID keyboard translation for alphanumeric, navigation, function,
  keypad and modifier keys
- mouse movement/buttons/wheel on player 1
- system/save directories
- libretro v0 variables/options
- save-state serialize/unserialize

Unknown environment calls intentionally return `false` until a core genuinely needs them.

## CPU policy

corehost never selects an interpreter. The emulator's own CPU backend remains authoritative.

For cores with JIT/dynarec/recompiler support, the PS5 build must keep the native x86-64 backend enabled. Full interpretation is only acceptable for systems whose chosen upstream core genuinely has no useful recompiler path.
