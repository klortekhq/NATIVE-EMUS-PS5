# Recompilation / decompilation reference audit — 2026-10-01

These sources were reviewed as engineering references for NATIVE-EMUS-PS5.
They are **not automatic dependencies** and do not change the rule that every
native emulator stays an independent port with its own upstream, licence and
validation gates.

## Discovery sources

### RetroPortingToolKit

The organization is useful because it now contains several actively maintained
static-recompilation frameworks plus shared runtime/frontend infrastructure.

The repositories pinned in `upstreams/scene-cache.json` are:

- `RetroPortingToolKit/psxrecomp@3505f2a...`
- `RetroPortingToolKit/snesrecomp@3fcd8b50...`
- `RetroPortingToolKit/ndsrecomp@c50d0992...`
- `RetroPortingToolKit/Retro-Runtime@5d5a83be...`

They are references for execution architecture, validation and host/runtime
boundaries. Per-game static recompilation is not treated as a replacement for a
general emulator.

### Recompendium / unricopie

`Nio03/unricopie@5e4b1ce5...` is a strong discovery index because it keeps
recomps, decomps, ports/remakes and tools in separate categories and refreshes
GitHub metadata automatically.

Use it to discover candidates, then inspect the canonical repository directly.
Do not treat catalogue descriptions or decompilation percentages as sufficient
evidence for integration.

### RecompCollection

The supplied RecompCollection page identifies playable recomp/decomp ports and
is useful as a second discovery list. Its visible snapshot is dated 2026-08, so
it is less suitable than live repository audits for deciding current pins.

### Curated GitHub stars

A user's/public developer's starred-repository page is useful as a discovery
feed only. Individual repositories must still be identified, pinned, licensed
and audited before entering the cache.

## Emulator-specific value

### PlayStation / Beetle PSX HW

`psxrecomp` is useful for:

- MIPS R3000A translation structure;
- LLE-first BIOS/runtime separation;
- guarded native dispatch;
- AOT overlay sharding for code loaded into reused RAM addresses;
- interpreter fallback that is intentionally reduced as native coverage grows;
- differential validation ideas.

It must not replace Beetle/Lightrec as the general emulator path. Its
PolyForm-Noncommercial licence also means no code import should happen without
an explicit licensing decision.

### SNES / Snes9x

`snesrecomp` is most valuable as a validation reference:

- 65816-to-C dispatch;
- shared hardware runtime;
- fallback interpreter for unresolved code;
- separate emulator oracle;
- frame/state differential comparison.

Our Snes9x title remains the general native emulator. The recomp framework can
inform testing and architecture without turning the emulator into a per-game
port.

### Nintendo DS

`ndsrecomp` demonstrates:

- ARM7TDMI + ARM946E-S ahead-of-time lifting;
- event-aligned dual-CPU scheduling;
- IPC/shared-memory coordination;
- bounded interpreter fallback for copied RAM code;
- exact-ROM dispatch gates;
- a separate melonDS-based oracle.

Its repository has mixed licensing boundaries (PolyForm, MIT and GPL-derived
areas), so all reuse must be reviewed per directory/component.

### PlayStation 2 / PCSX2

`ran-j/PS2Recomp@c5a9d025...` provides a useful independent R5900/IOP
reference:

- R5900 instruction translation;
- MMI and VU0 handling;
- function registration/dispatch;
- syscall routing;
- IOP R3000A execution plus HLE fallbacks.

This is a cross-check for PCSX2 CPU/IOP work and possible no-JIT experiments,
not a replacement for PCSX2.

### PlayStation 3 / RPCS3

`sp00nznet/ps3recomp@a679051e...` is now a **critical** research reference.

It independently models:

- PPU PowerPC state and instruction lifting;
- SPU register/local-store/channel/DMA behavior;
- per-image SPU dispatch;
- LV2 syscall/HLE boundaries;
- PRX/NID analysis;
- RSX-facing abstractions.

This is especially useful for checking our RPCS3 PPU/SPU and service semantics.
Its static per-game architecture cannot validate RPCS3's dynamic JIT allocation
contract, so the physical 2 GiB sparse-JIT probe remains the active RPCS3 gate.

### GameCube / Wii / Dolphin

`aharonahdoot/RecompCore@283baee0...` is a particularly valuable Dolphin
reference because it keeps Dolphin's hardware model and adds a static
recompilation CPU core with transparent interpreter fallback and differential
testing.

Use it to study:

- PowerPC native-module dispatch;
- interpreter fallback contracts;
- lockstep differential testing;
- JIT-vs-static performance measurement;
- a thin-fork strategy that minimizes divergence from Dolphin upstream.

`GRAnimated/Hagi@a1d1740e...` is also useful historical architecture evidence:
Hagi is Nintendo's GameCube/Wii emulator from Super Mario 3D All-Stars, and Wii
titles using it contain natively recompiled code. No usable licence was detected
in the audited tree, therefore it remains read-only research material.

### PowerPC cross-reference

`rexglue/rexglue-sdk@c94f5ebd...` and the XenonRecomp lineage are valuable
PowerPC code-generation references rooted in Xenia. Their guest is Xbox 360,
not PS3/GameCube, so instruction semantics and platform runtime behavior must
never be copied across blindly. Their portable-C++ codegen and release
engineering are still relevant comparison points.

## Shared runtime reference

`Retro-Runtime` is MIT and provides a clean example of separating:

- core ABI;
- host/runner IPC;
- overlays;
- savestate envelopes;
- core loading;
- versioned protocol negotiation.

This is useful for our long-term launcher/runtime architecture, particularly if
we later want one common native launcher while keeping every emulator repository
and engine independent.

## Licensing rule

Reference status never implies permission to copy code.

- MIT/BSD references may be candidates for reviewed reuse.
- GPL references require compatibility review before integration.
- PolyForm Noncommercial references stay research-only unless the project's
  distribution model is explicitly confirmed compatible.
- Repositories without an explicit licence stay read-only research references.

Every future import still needs a file-level provenance and licence review.
