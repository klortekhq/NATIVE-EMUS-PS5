# Roadmap

## Phase 0 — Repository foundation

- [x] Publish project goals and anti-Frankenstein rules.
- [x] Document architecture.
- [x] Track scene references.
- [x] Separate verified external work from repository implementation claims.
- [x] Recover and document M8 history.
- [x] Restore the complete 43-target M8 catalog.
- [x] Add the five additional systems discussed outside/after M8.
- [x] Create emulator/core workspaces for every current target family.
- [x] Add a machine-readable target manifest.
- [x] Add the ps5rt public C++ contract.
- [x] Add a public-header compile smoke test.
- [ ] Add issue templates / contribution workflow.
- [ ] Add pinned upstream revision manifest.
- [ ] Add complete license inventory.

## Phase 1 — PS5 shared runtime implementation

The API contract exists. The remaining work is the concrete PS5 backend.

### Memory / JIT
- [x] public memory/JIT API contract
- [x] flexible-memory implementation
- [x] direct-memory implementation
- [x] pooled-memory implementation
- [x] JIT shared-memory implementation
- [x] dual RW/RX mapping
- [x] instruction-cache flush implementation
- [x] large virtual-range reservation
- [x] sparse fixed-address direct-memory arena
- [ ] physical-PS5 sparse-arena validation
- [x] direct/flexible/pool availability diagnostics
- [x] runtime memory pressure diagnostics (largest direct block + tracked mappings/arenas)
- [ ] physical-PS5 memory diagnostics capture
- [ ] full virtual-address-space fragmentation walk
- [ ] configurable JIT cache sizing

### Threading / TLS
- [x] public thread/TLS API contract
- [x] current-thread name/affinity/stack-query PS5 backend + cross-build probe
- [x] PS5 JIT/recompiler stack-size recommendation policy + host regression test
- [x] explicit PS5 thread-creation stack-size implementation + join/detach contract
- [x] compiler-rt emulated TLS compile/link gate (`-femulated-tls` + SDK libc `__emutls_get_address`)
- [ ] execute emulated-TLS thread-isolation probe on physical PS5
- [ ] thread registration / cleanup
- [ ] thread naming/diagnostics
- [ ] shutdown ordering helpers

### Native services
- [x] public audio/input/VFS/app contracts
- [ ] AudioOut backend
- [ ] DualSense backend
- [ ] multi-user controller enumeration
- [ ] keyboard/mouse optional backend
- [ ] filesystem/storage discovery
- [ ] SMB VFS rebase from recovered PS5SX2 patch
- [ ] logging/crash reports
- [ ] user service initialization

### Vulkan
- [x] public bootstrap contract
- [ ] choose and pin PS5 Mesa/RADV baseline
- [ ] minimal Vulkan device/triangle test
- [ ] shader cache location policy
- [ ] command submission metrics
- [ ] readback metrics
- [ ] threaded recording evaluation

## Phase 2 — modern native emulator ports

### Flycast
- [x] pin upstream
- [x] build real rec-x64 core with PS5 toolchain
- [x] x86-64 SH4/ARM7/DSP dynarecs routed through ps5rt JIT/VM adapter at build time
- [ ] execute exact-capacity JIT + fastmem probes on physical PS5
- [ ] Vulkan through an audited public PS5 Vulkan/RADV baseline
- [ ] AudioOut
- [ ] DualSense
- [ ] BIOS/game VFS
- [ ] Dreamcast BIOS boot
- [ ] retail game boot
- [ ] Naomi/Atomiswave follow-up

### RPCS3
- [x] pin canonical RPCS3 + LLVM + AsmJit CPU stack
- [x] preserve historical PS5_RPCS3 / PS5_LLVM / PS5_Mesa provenance
- [x] reproduce SCE LLVM alignment defect
- [x] apply deterministic SCE-only alignment shim
- [x] cross-build real JITASM + SPU AsmJit + PPU LLVM + SPU LLVM engine objects
- [x] document PPU/SPU JIT memory requirements
- [x] implement and cross-build 2 GiB sparse-JIT arena probe
- [ ] execute sparse-JIT arena probe on physical PS5
- [ ] route canonical RPCS3 JITASM/JITLLVM arenas through ps5rt
- [ ] audit current public PS5 Vulkan/RADV references for RSX
- [ ] isolate Qt/desktop frontend
- [ ] validate headless/native title bootstrap
- [ ] reproducible full-engine build notes

### Vita3K / Azahar
- [x] pin canonical Vita3K upstream + exact Vita3K Dynarmic
- [x] pin canonical Azahar upstream + exact Azahar Dynarmic
- [x] keep emulator-specific Dynarmic revisions while sharing ps5rt JIT contracts
- [x] Vita3K A32 x86-64 Dynarmic engine cross-build
- [x] Azahar A32 sparse x86-64 Dynarmic engine cross-build
- [x] sparse W^X protection across multiple committed chunks
- [ ] physical PS5 JIT / sparse-memory validation
- [ ] Azahar core cross-compile against pinned CPU engine
- [ ] Vita3K core cross-compile against pinned CPU engine
- [ ] Vulkan bring-up
- [ ] native audio/input/VFS
- [ ] first guest boot

### Dolphin / Cemu / PPSSPP / Mupen64Plus / DeSmuME / PS1 / Saturn
- [ ] inventory PS5 scene deltas
- [ ] separate libretro/frontend glue from true PS5 platform fixes
- [ ] standalone native entry points
- [ ] consume ps5rt services
- [ ] reproducible builds

### xemu / Xenia
- [ ] isolate host platform requirements
- [ ] audit current Vulkan requirements against PS5 RADV
- [ ] adapt native audio/input/VFS
- [ ] establish first guest boot milestones

## Phase 3 — portable and preservation cores

### Nintendo / handheld
- [ ] Mesen 2
- [ ] Snes9x
- [ ] SameBoy
- [ ] mGBA

### Sega
- [ ] Genesis Plus GX
- [ ] PicoDrive

### Mednafen / Beetle family
- [ ] Virtual Boy
- [ ] WonderSwan/Color
- [ ] Lynx
- [ ] PC Engine / CD / SuperGrafx
- [ ] PC-FX
- [ ] Neo Geo Pocket/Color

### Arcade / computers
- [ ] MAME
- [ ] FinalBurn Neo
- [ ] VICE
- [ ] blueMSX
- [ ] Fuse
- [ ] Gearcoleco
- [ ] Stella
- [ ] Atari800
- [ ] ProSystem
- [ ] Virtual Jaguar
- [ ] NeoCD
- [ ] FreeIntv
- [ ] PUAE

The purpose of these ports is not only coverage. They are the safest place to harden the common PS5 host layer before every service is depended on by modern JIT-heavy emulators.

## Phase 4 — packaging and UX

Only after cores are stable:

- one native executable/package per system where practical;
- unified **optional** launcher;
- per-system artwork;
- per-game settings;
- common save/config paths;
- performance overlay;
- update mechanism;
- controller profiles;
- network storage;
- mobile/WebUI advanced configuration.

The launcher remains optional. No emulator core should require another emulator frontend to run.
