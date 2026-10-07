# PlayStation 1 emulator workspace

## Progress: **86%**

Selected implementation: **Beetle PSX HW + Lightrec**.

Canonical detailed status: [ports/beetle-psx-ps5](../../ports/beetle-psx-ps5/README.md).

## Architecture

```text
R3000A
  -> Lightrec
  -> GNU Lightning x86-64 threaded recompiler
  -> ps5rt executable memory
  -> PS5 Zen 2

PS1 GPU
  -> Beetle PSX HW Vulkan
  -> PS5 RADV
  -> PS5 GPU
```

### CPU rule

A final PS5 build with Lightrec disabled does **not** count as complete.

Interpreter execution is accepted only as a temporary diagnostic path.

## Current source

```text
libretro/beetle-psx-libretro@ed87921996c67658d7a70814f73034bbca08786a
```

Our deterministic transform owns the PS5 executable-memory and static-host adaptation. The old PS5_BeetlePSX fork is no longer a build dependency.

## Verified gates

Workflow **36833195325** completed successfully from the current public upstream; the previous engine artifact **11144874334** remains the documented binary evidence until the next uploaded engine artifact is recorded.

Verified in that run:

- Lightrec + GNU Lightning x86-64;
- threaded compiler;
- Zen 2 / SSE4.1 / AVX2 build;
- Vulkan RHI;
- native Vulkan host negotiation tests;
- AudioOut, DualSense and rumble adapter;
- content/save persistence tests;
- per-LBA native sector-size regression tests;
- validated local multi-disc M3U resolution;
- standalone shell compilation.

The 2026-10-01 hardening pass additionally supplies:

- immutable RADV bundle freeze/validation tooling;
- 2048/2336/2352 + mixed-mode optical-sector contracts;
- explicit region-aware BIOS boot defaults with OpenBIOS fallback left upstream;
- one shared FSELF finalizer path with PS1's AGC stubs;
- a repaired dedicated `ps1-lightrec` workflow.

Workflow **37160191512** completed the immutable pinned RADV bundle, full
standalone native PS5 link and FSELF finalization. The resulting artifact
contains the final linked PIE plus `eboot.elf` and `eboot.bin`, while CI
rechecks that the Lightrec/GNU Lightning x86-64 backend survives the final
engine. The score advances to **86%**: the title-link gate is complete and the
RADV integration gate advances, while physical boot/rendering and legal
test-content validation remain unproven.

Latest evidence: workflow **36835002466** succeeded, including save-state and
disk-control tests plus the native Lightrec x86-64 PS5 engine cross-build
(artifact **11148931941**).


Latest full-title evidence: workflow **37160191512**, artifact **11287755203**,
ZIP SHA-256
`b1ee45cc9afc153dc23d166c11a5b113d8924af54f46029bc606d8acc1653e50`.
No physical-console success is implied by this build evidence.
