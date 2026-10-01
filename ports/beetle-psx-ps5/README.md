# Beetle PSX HW PS5 — native Lightrec x86-64 port

## Progress: **67%**

> This percentage describes **our native standalone PS5 port**, not Beetle PSX upstream compatibility or another frontend's PS5 port. It only increases on reproducible engineering evidence.

### Progress accounting

| Gate | Weight | Current |
|---|---:|---:|
| Public upstream pinned + provenance mapped | 10% | **10%** |
| Reproducible current PS5 engine cross-build | 15% | **8%** — public-upstream migration is in CI; prior donor archive remains historical evidence |
| Native CPU recompiler/JIT | 25% | **22%** — Lightrec/GNU Lightning x86-64 path proven previously; current upstream gate pending |
| Native Vulkan host / RADV | 15% | **11%** — environment, provider, display surface and presenter implemented and host-tested |
| Native audio/input | 10% | **8%** — AudioOut, DualSense, rumble and clean-exit chord wired and host-tested |
| VFS/disc/saves | 10% | **5%** — local CUE/CHD/PBP/etc. validation + atomic SRAM persistence; network VFS pending |
| Native app shell / title link | 5% | **3%** — standalone lifecycle exists; PS5 shell compile/link gate is being closed |
| Physical PS5 boot/game validation | 10% | **0%** |

**Total: 67 / 100**

When the new public-upstream Lightrec cross-build goes green, the engine and CPU rows are rescored from that current evidence rather than from the historical donor.

## Canonical source

Current source pin:

```text
libretro/beetle-psx-libretro@ed87921996c67658d7a70814f73034bbca08786a
```

The previous `mihawk-99/PS5_BeetlePSX@e43b398...` build is retained only as historical PS5 feasibility/evidence. The repository stopped being publicly cloneable on 2026-10-01, so NATIVE-EMUS-PS5 no longer depends on it.

Our PS5 adaptation is a deterministic transform in:

```text
tools/ps5/apply_beetle_psx_ps5.py
```

That keeps the PS5 delta inspectable and lets us follow current Beetle fixes instead of freezing an inaccessible fork.

## CPU policy

The release CPU path is non-negotiable:

```text
PS1 R3000A
  -> Beetle PSX
  -> Lightrec
  -> GNU Lightning x86-64
  -> ps5rt executable code pool
  -> PS5 Zen 2
```

The standalone shell explicitly sets:

```text
beetle_psx_hw_cpu_dynarec = execute
```

The Beetle interpreter and Lightrec interpreter are not release backends. They may only be used as diagnostics.

### PS5 memory strategy

Desktop Lightrec normally tries POSIX/Win32 mappings for guest RAM mirrors and the code buffer. On PS5:

1. guest RAM, BIOS and scratch use Beetle's safe heap-backed fallback;
2. the Lightrec code pool is allocated with `ps5rt_exec_allocate()`;
3. GNU Lightning emits native x86-64 into that executable pool;
4. `ps5rt_exec_release()` owns teardown;
5. no Linux mmap/memfd compatibility layer is carried into the title.

## Graphics path

```text
Beetle PSX HW Vulkan RHI
        |
        v
native VulkanEnvironment / VulkanProvider
        |
        v
native VulkanPresenter + VK_KHR_display
        |
        v
PS5_Vulkan / RADV release archive
        |
        v
PS5 GPU + VideoOut
```

The core requests its hardware context during `retro_load_game()`. The standalone host accepts that request, creates/publishes the PS5 Vulkan context after the load call returns, and invokes the core's `context_reset`.

Current upstream also contains recent queue-ownership fixes: shared `VkQueue` submissions are serialized while waits happen outside the frontend queue lock. Our provider/presenter follows the same ownership model.

## Audio, input and lifecycle

`native/runtime_io.*` provides:

- native `libSceAudioOut` through `ps5rt::AudioDevice`;
- DualSense through `ps5rt::input`;
- rumble via the libretro rumble interface;
- Vulkan environment extension callbacks;
- **Options + Touchpad** as the clean standalone exit chord.

`native/main.cpp` now implements the complete standalone order:

1. initialize PS5 application services;
2. resolve content;
3. force Vulkan + Lightrec execute mode;
4. initialize the static core;
5. initialize native input/audio;
6. load content;
7. restore SRAM;
8. initialize/publish the Vulkan context;
9. run frames;
10. save SRAM periodically and on exit;
11. unload the core while its Vulkan device is still live;
12. shut down provider/runtime/app services.

## Content and saves

Current local content types:

- CUE
- CHD
- PBP
- ISO
- CCD
- TOC
- M3U
- PS-X EXE

Default writable tree:

```text
/data/NATIVE-EMUS-PS5/ps1/
  system/
  saves/
  boot.txt
```

`boot.txt` may contain a local content path when the title is launched without an argument.

Save RAM is written through a temporary file and rename, avoiding partially-written memory cards when an ordinary write fails.

Network URIs are deliberately rejected until the `ps5rt` VFS bridge is complete; the shell does not pretend SMB is working when it is not.

## Historical engine evidence

The old donor workflow **36781556800** produced artifact **11127512766**:

- size: 20,130,670 bytes;
- SHA-256: `b709412d7cc3dbe815fcde63dc6daddfdf994fe96998a12ed148cc857df2deb2`;
- contained `lightrec.o`, `recompiler.o`, `lightning.o`, `jit_memory.o`;
- exported `lightrec_execute` and GNU Lightning `_jit_set_code`;
- referenced `ps5rt_exec_allocate` and `ps5rt_exec_release`.

That evidence is preserved, but the current build must reproduce the same architecture from public upstream before we count the full engine/JIT gates again.

## Remaining gates

- make the current public-upstream PS5 archive green in CI;
- compile and link the standalone shell with the current engine;
- link a pinned PS5_Vulkan/RADV release archive using its published link recipe;
- convert the linked ELF into the native title/FSELF layout;
- boot on physical PS5;
- verify BIOS/OpenBIOS, legal test executable, CUE/CHD/PBP and save persistence;
- performance/compatibility pass with the native Lightrec backend.
