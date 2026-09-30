# Vita3K → native PS5 port map

Upstream reviewed: `Vita3K/Vita3K` at `df9b18ee645043fca88d17b95b0991bde5977bb1` (current head observed 2026-09-30).

The target is a native PS5 build that keeps Vita3K's Dynarmic CPU backend, GXM shader translation and Vulkan renderer intact while replacing desktop/platform services.

## 1. CPU — Dynarmic A32

Primary files:

- `vita3k/cpu/src/dynarmic_cpu.cpp`
- `vita3k/cpu/include/cpu/impl/dynarmic_cpu.h`
- `vita3k/cpu/include/cpu/impl/interface.h`

Vita3K constructs `Dynarmic::A32::Jit` through `Dynarmic::A32::UserConfig` and configures:

- ARMv7 guest architecture;
- a shared exclusive monitor;
- per-core processor id;
- safe optimizations;
- either a page table or a direct fastmem pointer.

The key upstream code path is already clean:

```text
Vita guest ARMv7
  -> Vita3K DynarmicCPU
  -> Dynarmic A32 JIT
  -> x86-64 code cache
  -> PS5 Zen 2
```

### PS5 strategy

Do not fork Vita3K's CPU core.

Use a PS5-capable Dynarmic build whose executable code cache is backed by the same host mechanism as `ps5rt::jit`.

Primary scene donor:

- `mihawk-99/PS5_Dynarmic`

Its PS5-specific code-cache work exists specifically to avoid consuming the title flexible-memory pool with per-core JIT caches.

The long-term goal is one PS5 Dynarmic solution shared by Vita3K, Azahar and Switch-family research.

## 2. Vita guest memory

Primary files:

- `vita3k/mem/src/mem.cpp`
- `vita3k/mem/include/mem/state.h`
- `vita3k/mem/include/mem/functions.h`

Current desktop implementation reserves Vita guest memory with `VirtualAlloc` or `mmap`, initially inaccessible, then controls page protection and access-violation handling.

It prefers a host address around `1ULL << 34`.

Vita3K can run Dynarmic in two important modes:

- `page_table` when renderer/mapping mode requires it;
- direct `fastmem_pointer` when page-table mode is not required.

### PS5 bring-up strategy

Do this in stages:

1. reserve the Vita guest address space with a PS5 memory backend;
2. validate normal page allocation/protection;
3. use Dynarmic page-table mode first if it reduces host fault complexity;
4. validate Vita3K's access-violation path on PS5;
5. enable direct fastmem only after the mapping model is proven.

The memory solution should use `ps5rt::memory` where the operation is truly generic, but Vita3K's guest page/protection semantics stay inside Vita3K.

## 3. Vulkan / GXM

Primary areas:

- `vita3k/renderer/src/vulkan/`
- `vita3k/renderer/include/renderer/vulkan/`
- `vita3k/shader/src/spirv_recompiler.cpp`
- `vita3k/shader/include/shader/`

Vita3K already translates Vita GXM/USSE shaders to SPIR-V.

It uses Vulkan-Hpp dynamic dispatch and can resolve functions through `vkGetInstanceProcAddr`.

### PS5 strategy

Keep the renderer and USSE→SPIR-V recompiler intact.

Add a PS5 display/surface bootstrap and feed Vita3K the selected PS5 Mesa/RADV Vulkan loader.

Do not convert GXM to AGC directly for this project branch; that would replace a proven upstream renderer with a new renderer and explode the delta.

Required audit before first frame:

- instance extensions;
- device features/extensions;
- formats, especially AMD-specific behavior;
- memory mapping mode;
- present/surface path;
- pipeline/shader cache persistence.

## 4. Audio

Primary files:

- `vita3k/audio/include/audio/state.h`
- `vita3k/audio/src/audio.cpp`
- existing SDL/Cubeb adapter implementations.

Vita3K already has the correct abstraction:

```cpp
class AudioAdapter {
  virtual bool init() = 0;
  virtual AudioOutPortPtr open_port(int channels, int freq, int samples);
  virtual void audio_output(AudioOutPort&, const void* buffer);
  virtual void set_volume(AudioOutPort&, float volume);
  virtual void switch_state(bool pause);
  ...
};
```

### PS5 strategy

Add `PS5AudioAdapter : AudioAdapter`, backed by `ps5rt::audio` / `libSceAudioOut`.

This removes both SDL and Cubeb from the PS5 audio path without touching guest audio emulation.

The adapter must honor Vita-requested channel count, frequency and sample count; resampling/format conversion belongs at this adapter boundary only when AudioOut cannot accept the guest-requested format directly.

## 5. Controllers, motion and rumble

Primary files:

- `vita3k/ctrl/include/ctrl/state.h`
- `vita3k/ctrl/src/ctrl.cpp`
- `vita3k/motion/src/motion.cpp`
- `vita3k/modules/SceCtrl/SceCtrl.cpp`
- platform polling currently present in `vita3k/app/src/app_init.cpp`.

Current host input is strongly coupled to SDL3 gamepad objects.

### PS5 strategy

The guest-facing SceCtrl logic remains untouched.

Introduce a native host-controller abstraction at the point where SDL states currently enter `CtrlState`.

Map from `ps5rt::InputSnapshot` to Vita controls:

- face buttons;
- D-pad;
- L/R + L2/R2 where VitaTV/extended behavior expects them;
- both analog sticks;
- PS/system shortcut outside guest input;
- rumble;
- motion sensors in a later step.

Do not retain SDL just to create controller objects.

## 6. Paths / filesystem

Primary path setup:

- `vita3k/app/src/app_init.cpp`
- `Root root_paths`
- renderer and logging already consume the `Root` abstraction.

This is favorable for PS5.

Create a PS5 path provider that sets, explicitly:

```text
vita_fs_path
log_path
config_path
shared_path
cache_path
patch_path
static_assets_path
```

Recommended policy:

```text
/data/NATIVE-EMUS-PS5/vita/
  ux0/
  config/
  cache/
  logs/
  patches/
```

with optional external storage later.

Keep Vita's emulated filesystem inside Vita3K. `ps5rt::vfs` only resolves host storage.

## 7. Frontend

Do not port Qt.

Vita3K already has non-Qt application/runtime layers and an Android frontend, proving the core is separable from desktop Qt.

First PS5 target should provide:

- native entry point;
- fixed/configured title path;
- minimal overlay/logging;
- PS5 input/audio;
- Vulkan surface;
- Vita install/user-data paths.

A richer shelf/WebUI follows after booting software.

## 8. Proposed PS5-specific source boundary

Prefer a narrow set such as:

```text
vita3k/platform/ps5/
  ps5_main.cpp
  ps5_paths.cpp
  ps5_audio.cpp
  ps5_input.cpp
  ps5_display.cpp
  ps5_memory.cpp
```

plus build-system conditions that exclude Qt/SDL host backends from the PS5 target.

Dynarmic PS5 support should live in the Dynarmic dependency / shared host layer, not in `dynarmic_cpu.cpp`.

## 9. Milestones

### V0 — PS5 core build
- C++23 toolchain works;
- Qt excluded;
- Vita3K core links;
- no SDL/Cubeb host dependency required for final target.

### V1 — memory + interpreter-equivalent core bootstrap
- guest address space reserved;
- Vita firmware/user paths initialized;
- application loader reaches early guest startup;
- logging stable.

### V2 — Dynarmic
- PS5 Dynarmic executable cache;
- all emulated Vita CPU cores create successfully;
- no flexible-memory exhaustion;
- page-table mode validated;
- fastmem evaluated after page-table mode.

### V3 — Vulkan
- Vulkan instance/device through PS5 RADV;
- USSE→SPIR-V pipeline compiles;
- first rendered frame.

### V4 — native services
- PS5AudioAdapter;
- DualSense input;
- rumble;
- save/config/cache paths.

### V5 — playable application
- stable guest threads;
- stable renderer;
- clean title stop/start;
- sustained gameplay validation.

## 10. Cross-project payoff

The most valuable result is not Vita3K-only.

If the PS5 Dynarmic path is clean, the same executable-memory/backend work becomes a direct dependency of:

- Vita3K;
- Azahar;
- Switch-family emulator research.

That is exactly the sort of platform code that belongs in the shared native PS5 foundation.
