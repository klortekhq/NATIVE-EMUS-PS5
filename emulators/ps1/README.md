# PlayStation 1 emulator workspace

## Progress: **30%**

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

## Next

The next score increase requires the PS5 cross-build job to produce a static archive that contains the Lightrec and GNU Lightning objects. After that the renderer and native service adapters become the next gates.
