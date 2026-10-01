# The Simpsons Game Recompiled — native PS5 port

Upstream: `YesterMester/TheSimpsonsGameRecomp`

Pinned revision: `15feabc95291a7a50851a9a184c5bd78d2dd1f4d`

## Goal

Run the existing ahead-of-time Xbox 360 PowerPC recompilation as a **native x86-64 PS5 title**.

This is not an Xbox 360 emulator port and it is not Xenia-on-PS5:

```text
Xbox 360 PPC default.xex
        |
        | XenonRecomp / existing generated C++
        v
~82k ahead-of-time recompiled guest functions
        |
        | prospero-clang++ / x86-64 / Zen 2
        v
native PS5 executable
        |
        +-- ReXGlue kernel/XAM/VFS shims
        +-- ReXGlue Vulkan renderer -> PS5 Vulkan display path
        +-- ReXGlue XMA decode -> ps5rt AudioOut
        +-- Xbox 360 input facade -> ps5rt DualSense
```

The generated guest functions remain upstream. The PS5 work lives in deterministic
platform transforms and an overlay in this repository so upstream can be updated
without hand-editing tens of thousands of generated functions.

## Rules

- No interpreter final path.
- No full-Xenia CPU emulation layer.
- Keep the existing AOT-generated x86-64 guest code.
- Reuse `ps5rt` for PS5 lifecycle, AudioOut, DualSense, storage and memory.
- Reuse ReXGlue's Vulkan Xenos renderer rather than writing a second Xbox 360 renderer.
- Use a PS5 Vulkan implementation with `VK_KHR_surface`, `VK_KHR_display` and
  `VK_KHR_swapchain`; link the driver into the title.
- Do not commit game data, keys, Sony SDK material or user dumps.
- A build/transform gate is not called hardware-playable until a real-console run proves it.

## Host / guest split

### CPU

The upstream project already translates the game's PowerPC code to C++ ahead of
time. PS5 is x86-64, so the final host compiler target is a good architectural
match. The port compiles the generated C++ with `prospero-clang++` for the PS5
Zen 2 CPU. There is no runtime PPC JIT requirement for the game code.

### Graphics

Keep ReXGlue's existing Vulkan backend and SPIR-V shader translation. The PS5
port adds a fixed fullscreen surface backed by `VK_KHR_display`, then lets the
normal ReXGlue swapchain/presenter continue above it.

The PS5 Vulkan driver is an external build dependency. It is intentionally not
vendored here. This repository only owns the ReXGlue adaptation and the link
integration.

### Audio

ReXGlue still performs Xbox 360/XMA-side work. The PS5 host audio driver converts
the six-channel guest frame to host stereo and submits 48 kHz float PCM through
`ps5rt::AudioDevice`, which owns the native `sceAudioOut` buffering.

### Input

The ReXGlue XInput-facing API remains unchanged. A PS5 input driver maps
`ps5rt::InputSnapshot` to Xbox 360 buttons, triggers and sticks, and maps guest
rumble back to DualSense.

### Files

The port expects legally obtained/extracted game data supplied by the user. The
runtime and package contain no game assets.

Default bring-up layout:

```text
/data/homebrew/TheSimpsonsGame/
  game/     # extracted game data, including default.xex
  user/     # saves/config
  cache/    # shader/runtime cache
```

The paths will remain compile-time overridable so final packaging can migrate
them without changing the ReXGlue core.

## Evidence gates

| Gate | Requirement | State |
|---|---|---|
| G0 | Pin upstream + deterministic transform | PASS (host-ci) |
| G1 | ReXGlue PS5 platform compiles with public PS5 SDK | PENDING |
| G2 | AOT Simpsons guest objects compile for x86-64 PS5 | PENDING |
| G3 | Native AudioOut + DualSense adapters link | PENDING |
| G4 | Vulkan instance/device + `VK_KHR_display` swapchain | PENDING |
| G5 | Full `simpsons_pie.elf` link and FSELF conversion | PENDING |
| G6 | Boot to first visible frame on physical PS5 | PENDING |
| G7 | Intro/FMVs/audio/input/gameplay smoke test | PENDING |
| G8 | PKG/directory-title packaging + save/cache persistence | PENDING |

Current status is **bring-up**, not playable. The pinned revision is also the current upstream `main` as of 2026-10-01, so the PS5 work is based on the latest available upstream fixes. G0 is CI-proven; the next gate cross-compiles the PS5-facing ReXGlue stack with the public Prospero toolchain.

## Build contract

The build path will follow the same repository-native model as the other modern
ports:

```text
pinned upstream
 -> apply_simpsons_recomp_ps5.py
 -> ReXGlue PS5 overlay
 -> prospero-clang / prospero-clang++
 -> native PIE
 -> finalize_native_pie.sh
 -> eboot.elf / eboot.bin
 -> directory title / package
```

`PS5_PAYLOAD_SDK` and `NATIVE_EMUS_ROOT` are mandatory. The Vulkan driver
archive is a separate, explicit input once the graphics link gate is enabled.

## Upstream update policy

Before changing the port:

1. inspect current upstream;
2. compare the pinned revision;
3. run the transform in `--check` mode;
4. update exact replacements only when upstream changed;
5. never silently apply fuzzy source edits.

That makes upstream drift fail loudly rather than producing an unreviewed
Frankenstein build.


## Bring-up commands

```bash
export NATIVE_EMUS_ROOT=/path/to/NATIVE-EMUS-PS5
export PS5_PAYLOAD_SDK=/path/to/ps5-payload-sdk
ports/simpsons-recomp-ps5/build_ps5.sh /path/to/TheSimpsonsGameRecomp
```

The build script applies `apply_simpsons_recomp_ps5.py` first. The transform is exact-match and idempotent: if upstream changes one of the blocks we depend on, it stops instead of applying a fuzzy patch.

### 2026-10-01 integration checkpoint

- upstream pinned and verified at `15feabc95291a7a50851a9a184c5bd78d2dd1f4d`;
- PS5 CMake/toolchain identity is `REXGLUE_PS5` / `ps5-amd64`;
- ReXGlue UI, audio and input source selection is redirected away from GTK/SDL host backends on PS5;
- `ps5rt::ps5` is wired as the native host service layer;
- SDL is skipped for the PS5 build path;
- DualSense `Create` maps to Xbox `Back/View`;
- the existing Vulkan renderer remains the graphics path; the next hard gate is a real PS5 Vulkan surface/device/swapchain link and then the full Simpsons executable link.
