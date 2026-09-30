# Sony PlayStation / PS1

## Progress: **30%**

> Progress is measured against the **native standalone PS5 target**, not against upstream Beetle PSX maturity.

| Gate | Weight | Current |
|---|---:|---:|
| Upstream/donor pin + provenance | 10% | 10% |
| Reproducible PS5 engine cross-build | 15% | 0% — CI running |
| Native Lightrec/GNU Lightning x86-64 recompiler | 25% | 10% — ps5rt executable-pool integration landed |
| Vulkan/RADV renderer | 15% | 5% — PS5 donor path verified, standalone integration pending |
| Native audio/input | 10% | 5% — ps5rt backends ready, PS1 adapter pending |
| Disc/VFS/saves | 10% | 0% |
| Native title/package shell | 5% | 0% |
| Physical PS5 boot/game validation | 10% | 0% |

**Total: 30 / 100**

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

1. green PS5 cross-build containing Lightrec + GNU Lightning;
2. native input/audio adapter;
3. standalone Vulkan/RADV renderer;
4. disc/VFS + memory-card persistence;
5. native app shell/package;
6. BIOS/legal test executable boot on physical PS5;
7. broader game validation.
