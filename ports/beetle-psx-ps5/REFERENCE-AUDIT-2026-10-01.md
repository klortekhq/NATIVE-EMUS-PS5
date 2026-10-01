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

Current native-port progress: **83%**.

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

### DuckStation native-sector regression audit — 2026-10-01 morning

Fresh upstream commits reviewed before this PS1 pass:

```text
3f037f2682284bde2ffab130bdda16760f9aac7b  CHD: copy the actual bytes-per-sector for each track mode
4d3dadd07e38acd1ea3f99f905a6db335bd2b26f  precache: retain each index's native sector format
e68431fda82f5800c782bf1b619b140a6cb472a2  PPF: calculate offsets/sizes from each track mode
```

The lesson is a **regression contract**, not a source-code donor: no PS1 host
layer may assume every data sector is 2352 bytes. Our current mixed-mode tests
already preserve 2048/2336/2352-byte sector widths by LBA, so no core rewrite
is needed. This audit is now part of the compatibility gate for CHD/CUE/PBP
work and future precache/network-VFS code.

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

The GitHub connector used by this workspace still returns 404 for Mihawk's
repositories, but public web access on 2026-10-01 confirms that **PS5_Vulkan**
and **PS5_RetroArch** are visible again. PS5_RetroArch now documents Beetle PSX
HW running on PS5 RADV, including hardware rendering at up to 16x internal
resolution, while PS5_Vulkan documents the native driver/tooling stack.

Because connector visibility and public visibility currently disagree, our build
must remain reproducible from exact pins and must not silently follow `main`.

### mihawk-99/PS5_RetroArch

Public web state reviewed 2026-10-01:

- Beetle PSX HW is documented as a working hardware-rendered core on PS5;
- Vulkan hardware contexts run through RADV linked into the native title;
- shader cache persistence is implemented;
- the driver is rebuilt and linked with the title rather than loaded from a
  desktop Vulkan stack;
- current public documentation reports Beetle PSX HW rendering at up to 16x.

This is strong **hardware proof for our chosen core/renderer pair**. We still do
not adopt RetroArch as the architecture: only its proven PS5 Vulkan/RADV
integration contract is relevant to our standalone frontend.

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


## Branch/state audit

Checked before this implementation pass:

- NATIVE-EMUS-PS5 exposes only the canonical `main` branch.
- the current PS1 workspace is already at 83%; work continues from that tree,
  not from an older checkpoint or side branch.
- `mihawk-99/PS5_BeetlePSX`, `PS5_Vulkan`, `PS5_Mesa` and
  `PS5_PayloadSDK` return 404 through the current GitHub API connection.
  Exact historical pins remain recorded; moving/unverified replacements are
  not substituted silently.
- `libretro/beetle-psx-libretro@ed87921996c67658d7a70814f73034bbca08786a`
  is still the newest reviewed canonical Beetle commit in this pass.

The full native-link workflow now also runs automatically for relevant PS1/RADV
changes on `main`, so every saved change exercises the next real gate rather
than relying on a manual dispatch.
