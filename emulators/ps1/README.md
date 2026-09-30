# PlayStation 1 emulator workspace

## Progress: **60%**

Selected implementation: **Beetle PSX HW + Lightrec**.

The progress score is shared with the canonical [PS1 system page](../../systems/ps1/README.md) and is based on weighted engineering gates, not a subjective estimate.

## Architecture

```text
R3000A
  -> Lightrec
  -> GNU Lightning x86-64
  -> ps5rt executable code pool
  -> Zen 2

PS1 GPU
  -> Beetle PSX HW Vulkan
  -> PS5 Vulkan / RADV
```

### Non-negotiable CPU rule

A final PS5 build with Lightrec disabled does **not** count as completed PS1 work for this project.

Interpreter execution may be used only as a diagnostic fallback while bring-up is in progress.

## Current implementation

The native port lives in:

- `ports/beetle-psx-ps5/`
- `tools/ps5/apply_beetle_psx_ps5.py`

The transform keeps the donor core recognizable and modifies only the PS5 executable-memory boundary needed by Lightrec.

## Current donor findings

The PS5 donor already provides:

- a PS5 platform target;
- Vulkan rendering work;
- PS5-specific disc precaching/performance tuning;
- the full Lightrec source tree, although its PS5 Makefile currently disables it.

Our port re-enables Lightrec and backs its TLSF code pool with `ps5rt`.

## Engine gate — complete

Workflow **36781556800** produced the PS5 engine archive (artifact **11127512766**).

Verified archive evidence:

- size: 20,130,670 bytes;
- SHA-256: `b709412d7cc3dbe815fcde63dc6daddfdf994fe96998a12ed148cc857df2deb2`;
- contains `lightrec.o`, `recompiler.o`, `lightning.o`, `jit_memory.o`;
- exports `lightrec_execute` and GNU Lightning `_jit_set_code`;
- references `ps5rt_exec_allocate` and `ps5rt_exec_release`.

This closes the engine cross-build and native CPU/JIT gates.

## Next

The next score increase now comes from the native Vulkan hardware-context provider and the PS1 audio/input adapter. The engine is already cross-built with the required x86-64 recompiler.
