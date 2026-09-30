# PPSSPP / PSP

## Progress: **25%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

## Status

**Native-engine extraction in progress.**

The port is pinned to upstream PPSSPP **v1.20.4** at commit
`fa50bb1976065c4f8b1b47af227d367fe9771555`, verified against the current stable release on 2026-09-30.

## CPU backend

Final backend: **PPSSPP MIPS x86-64 JIT**.

The PS5 port must expose executable memory and the 4 GiB mirrored guest-memory layout required by PPSSPP. Interpreter fallback is diagnostic only and is not an accepted final mode.

## Current repository work

- deterministic engine transformer: `tools/ps5/apply_ppsspp_native.py`
- PS5 toolchain: `tooling/ppsspp/ps5-engine-toolchain.cmake`
- exact-source verifier: `tools/ps5/prepare_ppsspp_native.sh`
- static engine compile gate: `tools/ps5/build_ppsspp_engine_archives.sh`
- shared C contracts for executable/shared/virtual memory under `runtime/include/ps5rt/c/`

The transformation keeps:

- native x86-64 MIPS JIT;
- executable-memory allocation through `ps5rt`;
- PPSSPP MemArena shared mappings;
- 4 GiB virtual-range reservation;
- Zen 2 host tuning;
- Vulkan-only direction;
- PPSSPP's own native lifecycle.

It deliberately excludes the libretro frontend.

## Native lifecycle

The intended title adapter uses PPSSPP's own API:

1. `CoreParameter.cpuCore = CPUCore::JIT`;
2. `PSP_InitStart()`;
3. `PSP_InitUpdate()` until boot completes;
4. `PSP_RunLoopFor()` / normal native frame path;
5. `PSP_Shutdown()`.

## Next gate

Implement the concrete PS5 `ps5rt_exec_*` / `ps5rt_shm_*` backend and link PPSSPP's Common/Core plus Vulkan graphics context into the first `emupsp` native title.
