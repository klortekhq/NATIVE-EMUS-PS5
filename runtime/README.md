# Shared PS5 Runtime

This directory will host the reusable PS5 platform layer for all native emulator ports.

## Planned modules

```text
runtime/
  include/ps5rt/
    app.hpp
    audio.hpp
    input.hpp
    io.hpp
    media.hpp
    jit.hpp
    memory.hpp
    thread.hpp
    tls.hpp
    vfs.hpp
    vulkan.hpp
  src/
    ...
```

## Scope

The runtime should provide only PS5 host services. Emulator-specific logic stays in each emulator tree.

### Memory / JIT

Target capabilities:

- flexible-memory mapping
- executable JIT memory
- direct/pooled memory where useful
- address-space diagnostics
- code-cache sizing
- cache invalidation helpers

Primary references:

- Swordpdf/PS5SX2
- mihawk-99/PS5_Dynarmic
- ps5-payload-dev SDK

### Vulkan / RADV

The runtime should expose only the minimum platform glue required for upstream Vulkan renderers.

Do not hide Vulkan behind a proprietary project abstraction that makes upstream merging harder.

Primary references:

- mihawk-99/PS5_Mesa
- mihawk-99/PS5_Vulkan
- Swordpdf/PS5SX2 Vulkan integration

### Threads / TLS

Desktop defaults are not always safe on PS5.

Required research:

- explicit pthread stack sizes
- emulated TLS via compiler-rt
- thread-local initialization
- affinity and scheduling
- clean shutdown ordering

### Audio

Preferred output is direct libSceAudioOut, with a small backend adapter.

### Input

Preferred host interface:

- DualSense
- multiple users/controllers
- rumble
- keyboard through USB-HID usage state
- mouse deltas/buttons/wheel

### Optical media

`ps5rt::media` provides a host-side disc source contract for systems that need random-access optical media without forcing a single generic disc parser onto every emulator.

Primary consumers include Sega CD, Saturn, Dreamcast, PC Engine CD, Neo Geo CD, PC-FX, CD-i, 3DO, PS1 and PS2.

Upstream CHD/CUE parsers remain inside the emulator when that is the cleaner/safer path; the shared layer mainly supplies storage/network readers.

### VFS

Normalize access to:

- /data
- application data
- USB
- external/M.2 locations
- optional future SMB

### Logging

Every port should support useful console-side diagnostics before performance tuning begins.

Minimum targets:

- boot log
- crash/signal log
- renderer timing
- JIT allocation report
- thread report
- optional per-frame performance counters

## Rule

If a new emulator needs a PS5-specific workaround that is not actually emulator-specific, it should graduate into this runtime instead of being copied into another port.
