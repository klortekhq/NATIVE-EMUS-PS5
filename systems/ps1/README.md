# Sony PlayStation / PS1

## Progress: **60%**

> Progress is measured against the **native standalone PS5 target**, not against upstream Beetle PSX maturity.

| Gate | Weight | Current |
|---|---:|---:|
| Upstream/donor pin + provenance | 10% | 10% |
| Reproducible PS5 engine cross-build | 15% | **15% — green** |
| Native Lightrec/GNU Lightning x86-64 recompiler | 25% | **25% — binary-verified** |
| Vulkan/RADV renderer | 15% | 5% — PS5 donor path verified, standalone integration pending |
| Native audio/input | 10% | 5% — ps5rt backends ready, PS1 adapter pending |
| Disc/VFS/saves | 10% | 0% |
| Native title/package shell | 5% | 0% |
| Physical PS5 boot/game validation | 10% | 0% |

**Total: 60 / 100**

The percentage only rises when a gate gains reproducible evidence.

## Selected engine

**Beetle PSX HW + Lightrec**

Primary PS5 donor:

- `mihawk-99/PS5_BeetlePSX@e43b3980e031c47066917c941be6ace6f51ed24f`

DuckStation/SwanStation remain useful comparison sources, but no equally strong public native-PS5 donor was found in the current audit.

## CPU policy

Final CPU path:

```text
R3000A guest
  -> Lightrec
  -> GNU Lightning x86-64
  -> ps5rt executable memory
  -> PS5 Zen 2
```

The interpreter is not the target release backend.

The current PS5_BeetlePSX donor deliberately disables Lightrec on its PS5 build because its executable code buffer had not yet been routed to console executable memory. NATIVE-EMUS-PS5 now owns that missing integration through `ps5rt_exec_allocate()`.

## Engine evidence

Workflow **36781556800** produced the PS5 engine archive (artifact **11127512766**).

Verified archive evidence:

- size: 20,130,670 bytes;
- SHA-256: `b709412d7cc3dbe815fcde63dc6daddfdf994fe96998a12ed148cc857df2deb2`;
- contains `lightrec.o`, `recompiler.o`, `lightning.o`, `jit_memory.o`;
- exports `lightrec_execute` and GNU Lightning `_jit_set_code`;
- references `ps5rt_exec_allocate` and `ps5rt_exec_release`.

This closes the engine cross-build and native CPU/JIT gates.

## Renderer

Target:

```text
PS1 GPU / Beetle PSX HW
  -> Vulkan renderer
  -> PS5 Vulkan / Mesa-RADV path
  -> PS5 GPU
```

The CPU/JIT cross-build is isolated first so renderer problems cannot hide recompiler problems.

## Media

Required final formats include the upstream-supported PS1 disc paths such as CUE/BIN, CHD and PBP where compatible.

Disc access will be integrated with the shared `ps5rt::media` / VFS layer while retaining upstream parsers when that is safer than replacing them.

## Workspaces

- [Native PS5 Lightrec engine gate](../../ports/beetle-psx-ps5/README.md)
- PS5 runtime: `../../runtime/`

## Immediate next gates

1. native input/audio adapter;
2. standalone Vulkan/RADV renderer;
3. disc/VFS + memory-card persistence;
4. native app shell/package;
5. BIOS/legal test executable boot on physical PS5;
6. broader game validation.
