# Beetle PSX HW PS5 — Lightrec x86-64 gate

## Progress: **60%**

> This percentage describes **our native standalone PS5 port**, not the maturity of Beetle PSX itself or Mihawk's existing PS5 libretro work.

### Progress accounting

| Gate | Weight | Current |
|---|---:|---:|
| Upstream/donor pinned + licensing/provenance mapped | 10% | 10% |
| Reproducible PS5 engine cross-build | 15% | **15% — green** |
| Native CPU recompiler/JIT | 25% | **25% — Lightrec + GNU Lightning x86-64 verified in archive** |
| Vulkan/RADV renderer | 15% | 5% — verified donor path exists; standalone integration pending |
| Native audio/input | 10% | 5% — shared ps5rt backends exist; PS1 adapter pending |
| VFS/disc/saves | 10% | 0% |
| Native title/package shell | 5% | 0% |
| Physical PS5 boot/game validation | 10% | 0% |

**Total: 60 / 100**

The score only increases when a gate has reproducible evidence.

## CPU policy

The final PS1 CPU path is:

```text
PS1 R3000A
  -> Beetle PSX / Lightrec
  -> GNU Lightning x86-64
  -> ps5rt executable code pool
  -> PS5 Zen 2
```

The interpreter is **not** the final CPU backend.

Mihawk's current PS5 donor deliberately disables Lightrec until its code buffer can come from PS5 executable memory. This port fills that gap by keeping Lightrec's existing TLSF code pool and supplying the pool from `ps5rt_exec_allocate()`.

## Why this is minimally invasive

Lightrec already calls `jit_set_code()` with chunks taken from its code pool. GNU Lightning therefore does not need to own the executable mmap when the pool exists.

On PS5 we:

1. leave PSX RAM/BIOS/scratch on the donor's safe fallback allocation path;
2. allocate only `LIGHTREC_CODEBUFFER_SIZE` through `ps5rt`;
3. give that pool to Lightrec;
4. keep the threaded recompiler enabled;
5. compile GNU Lightning for the PS5 x86-64 host.

No Linux/Wine wrapper and no interpreter-only release target.

## Pinned donor

`mihawk-99/PS5_BeetlePSX@e43b3980e031c47066917c941be6ace6f51ed24f`

This donor already contains the PS5 Vulkan platform work and recent PS5-specific disc-I/O tuning. We keep those as references while moving toward an independent native app.

## First gate

`build_engine_ps5.sh` must produce:

`libbeetle_psx_hw_ps5_lightrec.a`

The build fails if the archive does not contain Lightrec, the threaded recompiler and GNU Lightning objects.

## Engine evidence

Workflow **36781556800** produced the PS5 engine archive (artifact **11127512766**).

Verified archive evidence:

- size: 20,130,670 bytes;
- SHA-256: `b709412d7cc3dbe815fcde63dc6daddfdf994fe96998a12ed148cc857df2deb2`;
- contains `lightrec.o`, `recompiler.o`, `lightning.o`, `jit_memory.o`;
- exports `lightrec_execute` and GNU Lightning `_jit_set_code`;
- references `ps5rt_exec_allocate` and `ps5rt_exec_release`.

This closes the engine cross-build and native CPU/JIT gates.

## Next gates

- attach `ps5rt::input` + `AudioOut`;
- attach `ps5rt::input` + `AudioOut`;
- retain Beetle's Vulkan renderer on PS5 RADV;
- wire CHD/CUE/PBP through native VFS/media;
- produce native ELF/FSELF/PKG shell;
- boot BIOS and a legal test executable on physical PS5;
- then validate commercial-game compatibility using user-provided discs/images.
