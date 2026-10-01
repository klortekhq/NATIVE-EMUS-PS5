# PS1 reference audit — 2026-10-01

This file is intentionally updated **before** changing the PS1 port. The rule is:
check current public references first, then change our code only when the change is
not already solved elsewhere.

## Current project state

Canonical PS1 engine:

```text
Beetle PSX HW + Lightrec + GNU Lightning x86-64
```

Release CPU policy:

```text
R3000A guest -> Lightrec -> GNU Lightning x86-64 -> PS5 Zen 2
```

Interpreter modes are diagnostic only.

Current native-port progress: **78%**.

## Native PS5 references reviewed

### Rufidj/psx_ps5

Repository:

```text
Rufidj/psx_ps5
latest reviewed commit: ce0658b813638c0511ce6c7b65d949c108f15b99
```

Useful as PS5 platform evidence:

- BIOS/region handling;
- AudioOut/SPU experiments;
- native game boot/debug logging;
- pad/input integration;
- PS1 bus/GPU/GTE behavior observed on real PS5.

**Do not use its CPU as our release backend.** Its current `psx_cpu.c`
executes decoded R3000A instructions through a large opcode switch, i.e. an
interpreter. It is therefore a platform/behavior donor only.

### soniciso1/LuaPSX

Repository:

```text
soniciso1/LuaPSX
latest reviewed commit: b65ad7e31e15721b9b0b2783153a010e25e496ee
```

Useful evidence:

- PS1 game boot on retail PS5;
- CUE/BIN and PBP content;
- audio/video/input;
- large-disc streaming;
- disc picker and switching;
- savedata experiments.

Architecture note: it runs through Luac0re / ps2emu userland shellcode, so it is
not the native-title architecture of NATIVE-EMUS-PS5. Reuse observations and
test cases, not the wrapper architecture.

### DuckStation upstream

Repository:

```text
stenzek/duckstation
reviewed 2026-10-01
```

Fresh changes worth converting into compatibility tests:

- CHD: choose sector size from the actual track/mode;
- PPF: handle non-mode2 sectors;
- precache/memory CD image: retain each sector's native format;
- MDEC output FIFO correctness.

DuckStation is not copied into the current Beetle port. These fixes are useful
as **cross-emulator regression cases** for our disc/media layer.

### blackbearreloaded/ProsperoEden

Latest reviewed commit:

```text
183439625ef1d686d531426022d371142b3819bd
```

Current native-platform lessons:

- reproducible whole-project build with pinned dependencies;
- one controller per signed-in PS5 user;
- fast/clean game shutdown;
- RADV static entrypoint alias:
  `vkGetInstanceProcAddr=radv_GetInstanceProcAddr`;
- explicit pthread/stdio/TLS RADV link requirements;
- RADV/CPU memory-window separation;
- package inventory and hash receipts.

Its dependency manifest currently pins:

```text
PS5_Vulkan 71026e7ec1951fe72ae5b9118ff0905216a4c220
PS5_Mesa   cedb774b27d089fa81f46add28d0a8c13ff0f7d2
PS5_PayloadSDK 95c08f27386fc698f6bbe21dde3030140a41d10b
```

The Mihawk repositories are not currently publicly cloneable from this GitHub
connection, therefore our PS1 build must not require live access to them.

## Decision

Keep **Beetle PSX HW + Lightrec** as the PS1 engine.

Use native PS5 projects above only for platform lessons and regression cases.

The next implementation gate is:

1. make the RADV dependency an immutable local bundle/receipt;
2. validate its exact driver + matching SDK/tooling before link;
3. build the full standalone PS5 ELF with Lightrec still present;
4. package/convert;
5. physical-console validation.

## Short-link note

The supplied `t.co/H5u5RjOIHI` redirect could not be resolved by the current
web fetcher. The GitHub audit above was performed independently against the
current public PS1/native-PS5 repositories so work does not block on the
short-link redirect.
