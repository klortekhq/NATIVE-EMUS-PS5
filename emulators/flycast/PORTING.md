# Flycast → native PS5 port map

Upstream reviewed: `flyinghead/flycast` at `e36e9df2dcc1487acdb1dc7725766f1f5ba029b5` (observed current master on 2026-09-30).

This maps the real upstream integration points. The target is a standalone native PS5 port, not a libretro wrapper.

## 1. CPU / JIT

### SH4

Primary files:

- `core/hw/sh4/dyna/driver.cpp`
- `core/hw/sh4/dyna/ngen.h`
- `core/rec-x64/rec_x64.cpp`
- `core/rec-x64/xbyak_base.h`
- `core/oslib/virtmem.h`

The current SH4 dynarec reserves 10 MiB main code cache + 1 MiB temporary code cache.

`Sh4CodeBuffer` does not own platform allocation. PS5 work belongs underneath Flycast's existing `virtmem` interface.

### AICA ARM7

- `core/hw/arm7/arm7_rec.cpp`
- `core/hw/arm7/arm7_rec.h`
- `core/hw/arm7/arm7_rec_x64.cpp`

The ARM7 recompiler uses a 4 MiB code cache and calls `virtmem::prepare_jit_block()`.

### AICA DSP

- `core/hw/aica/dsp_x64.cpp`

The x64 DSP recompiler uses Xbyak and a 32 KiB code buffer, again through `virtmem::prepare_jit_block()`.

### PS5 strategy

Do not patch the three recompilers independently.

Implement Flycast's existing platform API:

- `virtmem::prepare_jit_block`
- `virtmem::release_jit_block`
- `virtmem::jit_set_exec`
- `virtmem::flush_cache`
- `virtmem::region_lock/unlock`
- `virtmem::region_set_exec`

backed by `ps5rt::JitRegion`, `create_jit_region()`, `destroy_jit_region()` and `flush_instruction_cache()`.

Prefer Flycast's existing `FEAT_NO_RWX_PAGES` route because it already understands separate writable/executable aliases. If the best PS5 implementation uses one JIT RWX mapping, the adapter can expose identical RW/RX addresses without changing the recompilers.

Immediate JIT validation should cover at least 11 MiB SH4 + 4 MiB ARM7 + 32 KiB DSP, plus alignment/metadata overhead.

## 2. Guest virtual memory / fastmem

Desktop reference: `core/linux/posix_vmem.cpp`.

Flycast's POSIX backend uses `mmap`, `mprotect`, shared mappings and a large reserved host range. The source documents a 512 MiB virtual address space for fast memory operations.

This is the main CPU-side porting risk after executable-code allocation.

Bring-up sequence:

1. boot with SH4 interpreter only to validate platform services;
2. implement PS5 `virtmem::init()` and guest mappings;
3. validate fault/context handling;
4. enable SH4 x64 dynarec;
5. enable ARM7/DSP recompilers;
6. compare interpreter/dynarec behavior on short deterministic runs.

Interpreter mode is only a smoke-test stage. Release architecture remains native x86-64 dynarec.

## 3. Vulkan

Primary files:

- `core/rend/vulkan/vulkan_context.h`
- `core/rend/vulkan/*`
- `core/rend/vulkan/oit/*`
- `core/rend/vulkan/vmallocator.cpp`

Standalone Flycast already exposes `VulkanContext::Create(void* window, void* display)` and uses normal Vulkan objects internally.

Keep the renderer intact. Add only the PS5 WSI/bootstrap necessary to connect it to the selected PS5 Mesa/RADV baseline.

Shared policy belongs in `ps5rt::vulkan`: driver/version pin, shader-cache path, presentation/native surface, queue selection, diagnostics and optional threaded-recording experiments.

## 4. Audio

Primary interface: `core/audio/audiostream.h`.

Flycast's backend contract is essentially `init()`, `push(data, frames, wait)` and `term()`.

The core produces stereo S16 at 44.1 kHz and `SAMPLE_COUNT` is 512 frames.

Add `PS5AudioBackend : AudioBackend` and route it to `ps5rt::audio` / `libSceAudioOut`.

First test whether the selected PS5 AudioOut mode accepts 44.1 kHz directly. If it requires 48 kHz, add one explicit 44.1→48 kHz conversion stage in the PS5 backend. Flycast's SDL backend already contains a 44.1→48 kHz fallback and is the behavioral reference.

## 5. Input

Useful files:

- `core/input/gamepad.h`
- `core/sdl/sdl_gamepad.h/.cpp`
- `core/sdl/sdl.cpp`

Do not keep SDL merely for controllers. Implement `PS5Gamepad` on Flycast's `GamepadDevice` abstraction and feed it from `ps5rt::poll_input()`.

Rumble routes through `ps5rt::set_rumble()`. Naomi/arcade-specific controls come after Dreamcast is stable.

## 6. Filesystem / games / saves / SMB

Primary interface: `core/oslib/storage.h`.

`hostfs::Storage` already abstracts directory listing, file open, parent/subpath handling, metadata and existence checks.

CUE/GDI/CDI readers already use `hostfs::storage()`. `core/chd.h` adapts `hostfs::File` into the `core_file` interface used by libchdr.

Implement `PS5Storage : hostfs::Storage` and a `PS5File : hostfs::File` adapter.

For network images, back `PS5File` with `ps5rt::RandomAccessReader`. This makes SMB a host filesystem capability rather than a Dreamcast-specific patch and allows GDI/CDI/CUE/CHD paths to benefit from the same random-access layer.

Credentials stay outside manifests and source.

## 7. Frontend / lifecycle

Do not port the complete desktop shell first.

Minimal PS5 entry point:

1. initialize `ps5rt::app`;
2. initialize storage/logging;
3. initialize native input/audio;
4. create Vulkan;
5. load one configured content path;
6. start Flycast;
7. cleanly release JIT/GPU/audio resources.

Only after this works should the game shelf/WebUI be connected.

## 8. Proposed upstream delta

- `shell/ps5/main.cpp`
- `shell/ps5/ps5_input.cpp`
- `shell/ps5/ps5_audio.cpp`
- `shell/ps5/ps5_storage.cpp`
- `shell/ps5/ps5_wsi.cpp`
- `core/ps5/ps5_vmem.cpp`
- `core/ps5/ps5_context.cpp`

If upstream structure suggests an even thinner arrangement during implementation, prefer the smaller delta.

## 9. Milestones

### F0 — toolchain
- core compiles for x86-64 PS5;
- no SDL/desktop window dependency in the target.

### F1 — interpreter smoke
- runtime initialized;
- BIOS/flash loaded;
- Vulkan created;
- Dreamcast BIOS reached with interpreter.

### F2 — JIT
- SH4 11 MiB cache;
- ARM7 4 MiB cache;
- DSP 32 KiB cache;
- no flexible-memory exhaustion;
- interpreter/dynarec short-run comparison.

### F3 — native I/O
- AudioOut;
- DualSense;
- saves/VMU;
- local/M.2/USB content.

### F4 — network storage
- `smb://` random access via shared ps5rt reader;
- GDI/CHD validation;
- reconnect/read-error behavior.

### F5 — Dreamcast playable
- sustained Vulkan rendering;
- stable audio and JIT;
- clean shutdown/restart.

### F6 — Naomi / Atomiswave
- arcade inputs;
- DIMM/content requirements;
- per-title validation.

## 10. No-go shortcuts

Do not:

- wrap the libretro core and call that the final standalone port;
- run a Linux Flycast build under another environment;
- disable dynarec permanently because interpreter boots first;
- duplicate SMB, AudioOut or DualSense code inside Flycast when the same host service belongs in ps5rt.
