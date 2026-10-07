# Dolphin → native PS5 port map

Upstream reviewed: `dolphin-emu/dolphin` at `5102a0339c2177575378107b76541e47cc52122d` (current head observed 2026-09-30).

The project target is a standalone PS5 Dolphin build for GameCube and Wii using Dolphin's native x86-64 JIT and Vulkan backend.

## 1. CPU — Jit64

Primary areas:

- `Source/Core/Core/PowerPC/Jit64/`
- `Source/Core/Core/PowerPC/Jit64Common/`
- `Source/Core/Core/PowerPC/JitCommon/`
- `Source/Core/Common/MemoryUtil.*`

Dolphin's x86-64 PowerPC backend is already exactly what PS5 needs:

```text
Gekko/Broadway PowerPC guest
  -> Dolphin Jit64
  -> x86-64 code
  -> PS5 Zen 2
```

Do not replace Jit64.

### Existing host-memory seam

Dolphin deliberately centralizes executable-code allocation in:

```cpp
Common::AllocateExecutableMemory(size)
Common::JITPageWriteEnableExecuteDisable()
Common::JITPageWriteDisableExecuteEnable()
```

and generic page protection in `MemoryUtil.cpp`.

This is the correct PS5 port seam.

### PS5 strategy

Add the PS5 implementation of Dolphin's existing memory primitives and back executable allocations with `ps5rt::jit`.

Dolphin already supports W^X-like host policies for Apple ARM, so its codebase is structurally prepared for a host where executable memory does not behave like ordinary desktop mmap.

Keep Dolphin's own nesting/state semantics; only substitute the underlying PS5 page/JIT mechanism.

## 2. Fastmem / fault handling

Dolphin performance relies heavily on fast guest memory access and exception/fault handling.

After basic JIT allocation works, audit:

- MMU fastmem mappings;
- memory protection;
- fault handlers;
- code invalidation;
- icache flush;
- page-size assumptions.

Bring-up may temporarily use a safer memory path, but the final native target should preserve the high-performance Jit64/fastmem architecture.

## 3. Vulkan

Primary area:

- `Source/Core/VideoBackends/Vulkan/`

`VulkanContext` already provides:

- instance creation;
- physical-device enumeration;
- device creation;
- queue selection;
- surface-aware initialization;
- feature/capability probing.

Its interface accepts a `WindowSystemType` and later a `VkSurfaceKHR`.

### PS5 strategy

Add a PS5 window-system type and surface/bootstrap implementation, then keep Dolphin's Vulkan backend intact.

Preferred shape:

```text
Dolphin Vulkan
  -> PS5 WindowSystemInfo / Vulkan WSI
  -> PS5 Mesa/RADV
```

Do not translate Dolphin Vulkan into AGC for this branch.

Validate PS5 RADV against Dolphin's required features before enabling enhancement options.

## 4. Native no-GUI frontend

Dolphin already has the ideal frontend reference:

- `Source/Core/DolphinNoGUI/`
- `Platform.h`

`Platform` requires only:

- `Init()`
- `MainLoop()`
- `GetWindowSystemInfo()`
- shutdown handling.

### PS5 strategy

Add `PlatformPS5`.

This is much cleaner than porting DolphinQt.

The first target should reuse DolphinNoGUI's boot path and provide a native PS5 event loop.

## 5. Audio

Primary interface:

- `Source/Core/AudioCommon/SoundStream.h`

Dolphin's `SoundStream` owns a 48 kHz mixer and provides a very small host abstraction:

- `Init()`
- `SetVolume()`
- `SetRunning()`

### PS5 strategy

Add `PS5SoundStream : SoundStream`, backed by `ps5rt::AudioDevice`.

48 kHz matches the strongest known PS5 AudioOut path and should minimize resampling complexity.

Preserve Dolphin's mixer and Wii Remote speaker mixing.

## 6. Input

Primary abstraction:

- `Source/Core/InputCommon/ControllerInterface/`
- `InputBackend.h`

Dolphin already supports multiple host input backends through `ciface::InputBackend`.

### PS5 strategy

Add a native `ciface::PS5` backend:

```text
ps5rt::InputSnapshot
  -> PS5 ciface Device
  -> Dolphin controller mapping
  -> GameCube Pad / Wii Remote emulation
```

Initial milestone:

- GameCube controller;
- Classic/Pro-style Wii mappings;
- rumble.

Later:

- DualSense gyro/accelerometer;
- Wii pointer emulation using gyro/touchpad;
- multiple controllers.

## 7. WindowSystemInfo

Dolphin already passes platform handles through `WindowSystemInfo`.

Current types include Headless, Windows, MacOS, Android, X11, Wayland, FBDev and Haiku.

Add `PS5` rather than pretending PS5 is Linux/FBDev.

This makes the backend explicit and prevents platform-specific hacks leaking into generic code.

## 8. Storage

Dolphin's emulated GC/Wii filesystems remain Dolphin-owned.

Map host paths to a stable root such as:

```text
/data/NATIVE-EMUS-PS5/dolphin/
  gc/
  wii/
  saves/
  cache/
  config/
  logs/
```

Games may live on internal, M.2 or USB roots exposed through `ps5rt::vfs`.

Network content should eventually use the shared random-access layer where Dolphin's disc/file abstraction permits it, rather than bespoke SMB code inside DVD emulation.

## 9. Proposed PS5-specific upstream delta

```text
Source/Core/DolphinNoGUI/PlatformPS5.cpp
Source/Core/InputCommon/ControllerInterface/PS5/
Source/Core/AudioCommon/PS5SoundStream.*
Source/Core/Common/PS5MemoryUtil.*       # or guarded MemoryUtil.cpp implementation
Source/Core/VideoBackends/Vulkan/PS5*   # minimal WSI/surface glue only
```

Prefer existing abstraction points over new wrappers.

## 10. Milestones

### D0 — build
- DolphinNoGUI core compiles for PS5 x86-64;
- DolphinQt not required;
- PS5 WindowSystemInfo exists.

### D1 — JIT
- executable memory allocated natively;
- Jit64 initializes;
- write/execute transitions or aliases validated;
- JIT smoke test executes generated host code.

### D2 — boot
- GameCube IPL-less title boot reaches guest code;
- then Wii title boot path.

### D3 — Vulkan
- PS5 RADV instance/device/surface;
- first frame;
- shader/pipeline cache persisted.

### D4 — native services
- PS5SoundStream;
- GameCube controller via DualSense;
- rumble;
- saves/configs.

### D5 — Wii UX
- motion;
- IR/pointer strategy;
- Wii Remote speaker;
- multi-controller validation.

## 11. Scene donor rule

Existing PS5 Dolphin/libretro work is valuable proof and a source of solved PS5-specific issues.

Extract platform fixes from it, but keep the final target structurally equivalent to DolphinNoGUI + native PS5 backends, not a mandatory RetroArch chain.
