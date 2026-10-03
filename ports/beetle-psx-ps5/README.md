# Beetle PSX HW PS5 — native Lightrec x86-64 port

## Progress: **83%**

> This percentage describes **our native standalone PS5 port**, not Beetle PSX upstream compatibility or another frontend's PS5 port. It only increases on reproducible engineering evidence.

### Progress accounting

| Gate | Weight | Current |
|---|---:|---:|
| Public upstream pinned + provenance mapped | 10% | **10%** |
| Reproducible current PS5 engine cross-build | 15% | **15% — green from public upstream** |
| Native CPU recompiler/JIT | 25% | **25% — Lightrec + GNU Lightning x86-64 verified in current archive** |
| Native Vulkan host / RADV | 15% | **11%** — environment, provider, display surface and presenter implemented and host-tested |
| Native audio/input | 10% | **8%** — AudioOut, DualSense, rumble and clean-exit chord wired and host-tested |
| VFS/disc/saves | 10% | **9%** — local media + multi-disc M3U, atomic SRAM, native EMUS HTTP Range and anchored remote descriptor sidecars implemented; physical network validation pending |
| Native app shell / title link | 5% | **4%** — standalone lifecycle + shell compile are green; final RADV-linked ELF/title conversion pending |
| Physical PS5 boot/game validation | 10% | **0%** |

**Total: 83 / 100**

Current public-upstream engine evidence is green.

### 2026-10-01 hardening

The percentage is now **83%**. The increase is limited to the disc/VFS/save gate because the
standalone path has now passed additional reproducible tests while the full RADV-linked app gate remains open:

- current native-PS5 references were audited before changing the port;
- the dedicated `ps1-lightrec` workflow syntax/allocator gate was repaired;
- RADV can now be supplied as an immutable, hash-validated bundle instead of a
  live clone of a currently unavailable graphics repository;
- native optical-sector metadata now preserves 2048/2336/2352-byte layouts and
  mixed-mode tracks;
- firmware policy is explicit: region auto, BIOS animation enabled, no forced
  region-free override; Beetle keeps its user BIOS search/OpenBIOS fallback;
- the standalone link now feeds its RADV AGC stubs into the shared pinned
  native-title/FSELF finalizer;
- the native `emus://` reader now transports descriptor-relative paths through
  SERVER-EMUS-PS5, so remote CUE/CCD/TOC/M3U layouts can keep ordinary relative
  BIN/IMG/SUB/disc references without SMB or physical NAS paths.

See [REFERENCE-AUDIT-2026-10-01.md](REFERENCE-AUDIT-2026-10-01.md).

The exact RADV source stack can now be materialized reproducibly with:

```bash
bash tools/ps5/prepare_radv_source_stack.sh
```

That script verifies the PS5_Vulkan, PS5_Mesa and PS5_PayloadSDK revisions used
by the current lock instead of following any moving branch. As of 2026-10-01
those exact historical Mihawk repositories/pins are not publicly retrievable.
The full-link workflow therefore has an explicit provenance preflight: it reports
the gate as **blocked** and does not claim link evidence until every exact pin is
available again (or an intentionally reviewed graphics-stack migration lands).

To build and freeze the actual immutable link bundle from those exact sources:

```bash
bash tools/ps5/build_radv_bundle_from_pins.sh
```

The resulting bundle is validated by hash and can be fed directly to
`build_app_ps5.sh` through `PS5_RADV_BUNDLE_DIR`.


Workflow `36833195325` validated, in one run:

- deterministic PS5 transform;
- Beetle Vulkan environment lifecycle;
- Vulkan negotiation/provider;
- PS5 `VK_KHR_display` surface selection;
- native presenter geometry;
- local content + SRAM persistence;
- per-LBA native sector-size preservation for 2048/2336/2352-byte mixed layouts;
- real local multi-disc M3U resolution/validation with network/nested playlists rejected until VFS is ready;
- AudioOut/input/rumble bridge;
- standalone native shell compilation;
- PS5 Lightrec x86-64 cross-build.

Artifact `11144874334` is `native-beetle-psx-hw-lightrec-x64-ps5-engine`.
The GitHub Actions ZIP digest is:

```text
sha256:0022701a3d1be329f3698b8eaaeda9d6a1cf3655995dc71aa68b0c6d4589c11f
```

The archive contains the current upstream Vulkan RHI plus `lightrec.o`,
`recompiler.o`, GNU Lightning `lightning.o` / `jit_memory.o`, and was
compiled with `ARCH_X86`, `ENABLE_THREADED_COMPILER=1`,
`-march=znver2`, SSE4.1 and AVX2.

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

boot.txt may contain a local content path when the title is launched without an argument.

Firmware belongs under:

```text
/data/NATIVE-EMUS-PS5/ps1/system/
```

Beetle retains its upstream region-aware search for `scph5500.bin`,
`scph5501.bin` and `scph5502.bin`. User firmware is never shipped by this
project; when no compatible user BIOS is present, the upstream OpenBIOS path
remains available.


Save RAM is written through a temporary file and rename, avoiding partially-written memory cards when an ordinary write fails.

The standalone shell accepts `emus://` through the native ps5rt HTTP backend.
The client performs HEAD metadata discovery and ranged GETs with ETag/If-Range.
A dropped native HTTP transport connection is retried once after reopening the
connection and refreshing HEAD/ETag; HTTP status errors are not retried as if
they were network failures.

Optional read-ahead is configured in `/data/NATIVE-EMUS-PS5/ps1/config.ini`:

```ini
server_read_ahead_kib = 0
server_max_read_ahead_kib = 0
```

`server_read_ahead_kib = 0` preserves exact-range reads. A non-zero base up to
8192 KiB enables one bounded in-memory read-ahead window per open remote file.
`server_max_read_ahead_kib = 0` preserves fixed-window behavior; a value above
the base enables the host-tested adaptive policy, which doubles the window on
consecutive logical reads up to that cap and resets to the base after a seek.

Both defaults stay disabled until physical-PS5 benchmarks establish useful
values. Compare SERVER-EMUS-PS5 saved read plans plus `/api/v1/metrics`
counters such as `range_requests` and `bytes_served` while testing candidate
values rather than assuming a universal prefetch size.

SERVER-EMUS-PS5 keeps the launchable catalog separate from descriptor sidecars:
a catalog ID anchors virtual relative paths for CUE/CCD/TOC/M3U companions, and
the server rejects path or symlink escapes outside the configured library.
Other network schemes are not silently treated as interchangeable transports.

### VFS bridge status

The host-side libretro bridge is now implemented and deliberately advertises
**VFS v1 read-only only**:

- Beetle may request v5/v4/v3 first; those requests are rejected;
- Beetle's hybrid VFS then falls back to v1;
- v1 `open/size/tell/seek/read` route through `ps5rt::RandomAccessReader`;
- local and `file://` reads are host-tested, including EOF and seek semantics;
- `smb://` and native `emus://` random-access backends are registered behind
  `ps5rt::open_random_access`;
- `emus://host/id/path` preserves one opaque catalog anchor while relative
  descriptor paths are percent-encoded and resolved server-side inside the same
  configured library;
- remote M3U preparation is covered by the PS1 content regression, and the
  EMUS URI translator has a dedicated host test;
- write/remove/rename are rejected instead of being partially advertised;
- the next VFS subgate is real PS5 seek/read/disc-swap validation over LAN plus
  throughput/latency comparison against SMB.

This deliberately **does not change the 83% score** yet. The existing VFS/disc/save
gate remains 9/10 until network random access passes real seek/read tests.


## Current engine evidence

The current canonical build is the public-upstream run above. It supersedes the old donor archive for progress scoring.

## Historical engine evidence

The old donor workflow **36781556800** produced artifact **11127512766**:

- size: 20,130,670 bytes;
- SHA-256: `b709412d7cc3dbe815fcde63dc6daddfdf994fe96998a12ed148cc857df2deb2`;
- contained `lightrec.o`, `recompiler.o`, `lightning.o`, `jit_memory.o`;
- exported `lightrec_execute` and GNU Lightning `_jit_set_code`;
- referenced `ps5rt_exec_allocate` and `ps5rt_exec_release`.

That evidence is preserved, but the current build must reproduce the same architecture from public upstream before we count the full engine/JIT gates again.

## Remaining gates

- restore access to the exact pinned historical RADV sources or deliberately migrate the graphics provider to a newly audited public stack;
- build/freeze/validate the resulting matching driver + SDK bundle and complete the full standalone Vulkan link with the current Lightrec engine;
- validate the generated native ELF/FSELF from that complete link;
- boot on physical PS5;
- validate regional BIOS/OpenBIOS with legal test content;
- validate local and SERVER-EMUS-PS5 CUE/CHD/PBP, mixed-mode discs, remote multi-disc M3U and memory cards on physical hardware;
- performance/compatibility pass with the native Lightrec backend.

## Standalone hotkeys

- **Options + R1** — save state slot 0.
- **Options + L1** — load state slot 0.
- **Touchpad + R1** — next disc.
- **Touchpad + L1** — previous disc.
- **Options + Touchpad** — exit cleanly.

Save/load and disc changes are edge-triggered, so holding a combination cannot
repeat the action every emulated frame. Disc swaps are deferred until after the
current frame and use Beetle's own disk-control sequence: eject, select image,
close tray.

## Latest verified CI evidence

Workflow **36844576020** completed successfully at
`8f91540c61d2d5f8fe7fc3076cfac7b504deb704`.

It validates the deterministic Beetle transform, standalone content path,
remote EMUS multi-disc preparation, the native Prospero build of the EMUS
random-access backend, guarded save-state/disc-control paths and the PS5
Lightrec x86-64 engine cross-build.

The last engine artifact recorded before this networking increment remains
**11148931941** from workflow **36835002466**; no new progress percentage is
claimed until physical-network and final Vulkan-title gates pass.
