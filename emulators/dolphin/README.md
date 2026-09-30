# Dolphin / GameCube · Wii

## Status

**Jit64/fastmem architecture proven externally; standalone native extraction active.**

Existing public PS5 work demonstrates Dolphin running actual GameCube code on the console with **JIT64 + fastmem + Vulkan**. NATIVE-EMUS-PS5 is extracting those platform fixes into a standalone title rather than carrying RetroArch as a runtime dependency.

## CPU policy

Final PS5 CPU backend: **Dolphin Jit64**.

```text
PowerPC guest
  -> Dolphin Jit64
  -> ps5rt executable memory
  -> PS5 Zen 2
```

Full interpretation is diagnostic only.

## Fastmem / memory

The selective platform patch maps Dolphin's existing abstractions onto the shared runtime:

- guest RAM and mirrors -> `ps5rt_shm_*`
- fastmem virtual arena -> `ps5rt_vrange_*`
- JIT code caches -> `ps5rt_exec_*`
- fixed aliases preserve holes/reservations
- fastmem faults handle both SIGSEGV and SIGBUS on PS5
- PS5 avoids Dolphin's 64 GiB large entry-points lazy mapping

The goal is to preserve Dolphin's JIT and MMU machinery, not invent a replacement CPU core.

## Graphics

```text
Flipper / Hollywood
  -> Dolphin Vulkan
  -> PS5 Mesa/RADV
```

AGC experiments from other native Wii projects remain useful reference material, but Vulkan/RADV is the first Dolphin renderer because it preserves upstream architecture.

## Current implementation in this repository

- exact donor pin recorded in `PS5-DONOR-PIN.md`
- selective 408-line platform patch:
  `patches/dolphin/c6630001-ps5-platform.patch`
- patch excludes RetroArch/frame-pacing/debug hunks
- PS5 headers retargeted to `ps5rt`
- deterministic patch gate:
  `tools/ps5/apply_dolphin_native.py`
- standalone engine toolchain:
  `tooling/dolphin/ps5-engine-toolchain.cmake`
- engine build script:
  `tools/ps5/build_dolphin_engine.sh`
- donor analysis:
  `DONOR-AUDIT.md`

## Current gates

- [x] Jit64 + fastmem PS5 hardware feasibility established externally
- [x] exact hardware-proven donor source pinned
- [x] platform-only patch extracted
- [x] JIT/fastmem patch retargeted to ps5rt
- [x] standalone toolchain/build path defined
- [ ] prospero-clang engine cross-build
- [ ] standalone title lifecycle
- [ ] Vulkan PS5 surface/device
- [ ] physical GameCube boot from our title
- [ ] physical Wii boot from our title

The final two gates are not claimed until they run from this repository's standalone build.
