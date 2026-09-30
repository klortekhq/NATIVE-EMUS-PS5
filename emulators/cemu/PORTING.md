# Cemu → native PS5 port map

Upstream reviewed: `cemu-project/Cemu` at `c717fcab1ccc3e0b0b97499a4d9b04a77084e347` (current head observed 2026-09-30).

The PS5 goal is a **headless-first native Cemu core**: Espresso PPC recompiler + Latte Vulkan renderer + Cafe system, with wxWidgets kept out of the initial target.

## 1. Espresso CPU / x64 recompiler

Primary areas:

- `src/Cafe/HW/Espresso/Recompiler/`
- `src/Cafe/HW/Espresso/Recompiler/BackendX64/`
- `src/Cafe/HW/Espresso/PPCScheduler.cpp`

Cemu already contains a dedicated x86-64 backend:

```text
Wii U PowerPC guest
  -> Cemu IML
  -> BackendX64
  -> PS5 Zen 2
```

The current recompiler allocates generated host code in blocks of at least **4 MiB** through:

```cpp
MemMapper::AllocateMemory(..., PAGE_PERMISSION::P_RWX)
```

The Wii U executable guest region itself is:

- `MEMORY_CODEAREA_ADDR = 0x02000000`
- `MEMORY_CODEAREA_SIZE = 0x0E000000` (224 MiB)

and the recompiler's guest code-address bookkeeping covers a 256 MiB range.

### PS5 strategy

Do not rewrite BackendX64.

Port Cemu's `MemMapper` host implementation so executable allocations use `ps5rt::jit`.

Two allocations must remain conceptually separate:

1. guest Wii U memory / mapping;
2. generated host x86-64 code.

The generated host code can use the same shared JIT-memory mechanism already needed by PCSX2/Flycast.

The recompiler jump-table allocations are RW data, not executable code, and should stay ordinary mapped memory.

## 2. Memory / MMU

Primary areas:

- `src/Cafe/HW/MMU/`
- `src/util/MemMapper/`
- RPL loader / code heaps.

Cemu has strong assumptions about guest address ranges and reserves/commits memory on demand.

### PS5 bring-up

1. port `MemMapper::ReserveMemory`;
2. port `MemMapper::AllocateMemory` for RW guest regions;
3. port executable host allocations through JIT memory;
4. validate fixed-address requirements individually;
5. bring up Espresso interpreter/recompiler;
6. run RPL loader and Cafe init before touching a full GUI.

Do not blindly emulate desktop `mmap` semantics when PS5 has a better native memory API.

## 3. Vulkan / Latte

Primary renderer:

- `src/Cafe/HW/Latte/Renderer/Vulkan/`
- `VulkanRenderer.h/.cpp`

Cemu already has a large Vulkan renderer, including:

- AMD/Mesa detection;
- format capability handling;
- surface/swapchain abstraction;
- memory manager;
- pipeline compilation;
- readback;
- shader caches.

The renderer explicitly recognizes Mesa RADV.

### PS5 strategy

Keep Latte→Vulkan intact.

Add a PS5 `WindowSystem::WindowHandleInfo` / surface implementation and route normal Vulkan loading to the selected PS5 Mesa/RADV build.

Useful shared PS5 concerns:

- driver pin;
- surface creation;
- present path;
- shader-cache location;
- GPU memory diagnostics;
- command-submission/readback profiling.

Cemu renderer semantics stay in Cemu, not in `ps5rt`.

## 4. Audio

Primary interface:

- `src/audio/IAudioAPI.h`

Cemu already abstracts output devices via `IAudioAPI`.

Important methods:

- `NeedAdditionalBlocks()`
- `FeedBlock(sint16*)`
- `Play()`
- `Stop()`
- volume control.

Wii U TV audio is created at 48 kHz in the Cafe audio path.

### PS5 strategy

Add `PS5AudioAPI : IAudioAPI` backed by `ps5rt::AudioDevice`.

This is a natural fit and avoids carrying Cubeb just for PS5.

Keep the distinction between TV/GamePad/Portal audio at the Cemu layer. The PS5 backend may initially map TV + GamePad to the same physical output, then improve routing later.

## 5. Input

Primary host abstraction:

- `src/input/InputManager.*`
- `src/input/api/ControllerProvider.h`
- `src/input/api/Controller.*`
- existing SDL provider as behavioral reference.

Cemu already has a provider model:

```cpp
ControllerProviderBase::get_controllers()
```

### PS5 strategy

Implement:

```text
PS5ControllerProvider
  -> PS5Controller
  -> ps5rt::InputSnapshot
```

Then let existing VPAD/WPAD code consume ordinary Cemu controllers.

This cleanly supports:

- GamePad buttons/sticks;
- Pro Controller mapping;
- rumble;
- future motion;
- multiple controllers.

Do not keep SDL as a mandatory PS5 input layer.

## 6. Core/frontend boundary

The key abstraction already exists:

`CafeSystem::SystemImplementation`

It only requires host callbacks such as:

- `CafeRecreateCanvas()`
- `CafePPCProcessExit()`

and Cafe itself exposes:

- `Initialize()`
- `PrepareForegroundTitle(...)`
- `PrepareForegroundTitleFromStandaloneRPX(...)`
- `LaunchForegroundTitle()`
- `ShutdownTitle()`
- `Shutdown()`

This is the exact seam needed for a PS5 frontend.

### PS5 strategy

Create a tiny `PS5SystemImplementation`.

Do **not** make `MainWindow : wxFrame` part of the first PS5 build.

The initial application should:

1. initialize platform services;
2. initialize Cafe;
3. configure MLC/content paths;
4. prepare one title/RPX;
5. initialize Vulkan;
6. launch;
7. poll native input;
8. handle process exit;
9. shut down cleanly.

## 7. Filesystem

Cemu's Wii U storage/MLC semantics stay in Cemu.

The host side should map a stable base such as:

```text
/data/NATIVE-EMUS-PS5/wiiu/
  mlc01/
  games/
  cache/
  config/
  logs/
```

External/M.2/USB content can be exposed through the path layer after local boot is stable.

SMB should be integrated only at a host file abstraction that can satisfy the access pattern required by Wii U content; do not inject SMB calls through Cafe OS code.

## 8. Proposed PS5 boundary

Prefer new PS5-specific files around existing abstractions:

```text
src/platform/ps5/
  PS5Main.cpp
  PS5SystemImplementation.cpp
  PS5WindowSystem.cpp
  PS5AudioAPI.cpp
  PS5ControllerProvider.cpp
  PS5MemMapper.cpp
  PS5Paths.cpp
```

The exact file placement should follow upstream Cemu conventions once the first build branch exists.

## 9. Milestones

### C0 — headless build
- C++ core builds with PS5 toolchain;
- wxWidgets excluded;
- Cafe initializes;
- logs persist.

### C1 — memory / RPL
- Wii U guest ranges reserve/commit correctly;
- standalone RPX can be prepared;
- title filesystem is visible.

### C2 — x64 recompiler
- 4 MiB+ executable code blocks allocate through PS5 JIT memory;
- recompiler interface functions execute;
- Espresso reaches real guest code.

### C3 — Vulkan
- PS5 RADV device selected;
- surface/swapchain created;
- Latte initializes;
- first frame.

### C4 — native services
- PS5AudioAPI;
- PS5ControllerProvider;
- saves/MLC;
- clean exit/relaunch.

### C5 — playable title
- stable PPC recompiler;
- stable Latte renderer;
- shader/pipeline cache persisted;
- sustained gameplay.

## 10. Main risk

Cemu's hardest PS5 problem is **not** its CPU architecture: it already has an x64 backend.

The main work is host-memory behavior + disentangling desktop window/UI startup + validating the mature Vulkan renderer against PS5 RADV.
