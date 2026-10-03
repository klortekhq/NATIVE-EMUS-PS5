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

- explicit pthread stack sizes (**implemented in the native PS5 thread factory; physical execution pending**)
- emulated TLS via compiler-rt (**ps5rt policy backend + `-femulated-tls` cross-build implemented; physical isolation probe pending**)
- thread-local initialization (**covered by the same three-thread synthetic probe; physical execution pending**)
- affinity and scheduling
- clean shutdown ordering

### Audio

The native backend now targets direct libSceAudioOut with a bounded stereo
ring, background grain submission, f32->s16 conversion, and stateful source
rate conversion to the 48 kHz hardware path. Host-mocked regression tests cover
open/close, pause/flush, clipping/conversion, and 24->48 kHz resampling.
Physical-console AudioOut behavior remains a separate validation gate.

### Input

Preferred host interface:

- DualSense
- multiple users/controllers
- stable local-player slots (`PLAYER1`–`PLAYER4`) remappable independently of sign-in order
- rumble
- keyboard through USB-HID usage state
- mouse deltas/buttons/wheel

`ps5rt::map_local_players` provides a portable local mapping contract: each
player selects one connected controller source or is disabled. It is a pure
offline operation and requires no network/session service. Invalid and
duplicate source assignments are rejected. Remote players belong to a later,
separate session-layer mapping and do not consume local controller indices.
Each core must still declare its actual player/port limit and adapt only the
slots its system supports.

The shared libretro `StaticCore` adapter activates four joypad ports and routes
each port to its corresponding local controller snapshot. This makes PLAYER1
through PLAYER4 inputs available to cores that implement those ports; an
emulator that supports fewer players still controls its own actual limit. Host
tests cover the port contract, while simultaneous-controller behavior on
physical PS5 hardware remains a separate validation gate.

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

- boot log (**persistent append-only file sink implemented; title chooses path**)
- crash/signal log (**still pending; must not steal JIT/fastmem fault handlers**)
- renderer timing
- JIT allocation report
- thread report
- optional per-frame performance counters

## Rule

If a new emulator needs a PS5-specific workaround that is not actually emulator-specific, it should graduate into this runtime instead of being copied into another port.


### Lifecycle / shutdown ordering

`ps5rt::ShutdownStack` is a bounded allocation-free LIFO cleanup sequence for
standalone native runners. Callbacks are idempotently drained by `run()` or
the stack destructor. The shared static-core PS5 runner now uses it while
preserving its established teardown order:

```text
core -> audio -> input -> video -> app/UserService
```

This is lifecycle cleanup only; it is intentionally independent of signal/crash
handling so it cannot consume emulator JIT fault signals.


### Thread diagnostics

`ps5rt::query_current_thread_diagnostics()` snapshots the current native
thread name, stack size and ps5rt-managed worker count using the same concrete
PS5 APIs as the lower-level thread contract. The native thread probe compiles
this combined path and verifies it agrees with the individual queries before
creating a managed worker. Physical execution of that probe remains a separate
hardware gate.


### JIT cache sizing policy

`ps5rt::JitCachePolicy` provides a portable, host-tested way for individual
emulator ports to expose cache-size overrides without accepting arbitrary or
unaligned executable-memory requests. A port defines its upstream-safe
default/minimum/maximum and alignment; `select_jit_cache_size()` clamps and
aligns an optional user/config override before the existing JIT allocator is
called.

The policy does not silently resize any emulator. Each port must opt in with
bounds appropriate to its own recompiler and physical-PS5 memory evidence.
