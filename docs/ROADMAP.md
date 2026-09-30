# Roadmap

## Phase 0 — Repository foundation

- [x] Publish project goals and rules.
- [x] Document architecture.
- [x] Track scene references.
- [x] Separate verified external work from repository implementation claims.
- [ ] Add issue templates / contribution workflow.
- [ ] Add upstream revision manifest format.
- [ ] Add license inventory.

## Phase 1 — PS5 shared runtime

### Memory / JIT
- [ ] executable-memory abstraction
- [ ] flexible-memory abstraction
- [ ] pooled/direct-memory helpers
- [ ] cache flush helpers
- [ ] address-space diagnostics
- [ ] configurable JIT cache sizing

### Threading / TLS
- [ ] explicit thread stack-size wrapper
- [ ] compiler-rt emulated TLS evaluation
- [ ] thread naming/diagnostics
- [ ] shutdown ordering helpers

### Native services
- [ ] AudioOut backend
- [ ] DualSense backend
- [ ] multi-user controller enumeration
- [ ] filesystem/storage discovery
- [ ] logging/crash reports
- [ ] user service initialization

### Vulkan
- [ ] choose and pin PS5 Mesa/RADV baseline
- [ ] minimal Vulkan triangle test
- [ ] shader cache location policy
- [ ] command submission metrics
- [ ] readback metrics
- [ ] threaded recording evaluation

## Phase 2 — Native emulator ports

### Flycast
Goal: first clean standalone port built around upstream Flycast.

- [ ] build core with PS5 toolchain
- [ ] x86-64 SH4 dynarec executable memory
- [ ] Vulkan renderer
- [ ] AudioOut
- [ ] DualSense
- [ ] BIOS/game VFS
- [ ] Dreamcast boot
- [ ] Naomi/Atomiswave follow-up

### RPCS3
Goal: cooperate with / learn from current PS5 scene work rather than duplicate it blindly.

- [ ] track PS5_RPCS3 upstream delta
- [ ] track PS5_LLVM ABI patches
- [ ] track PS5_Mesa RSX optimizations
- [ ] document PPU/SPU JIT memory requirements
- [ ] remove desktop-only frontend assumptions
- [ ] validate headless boot path
- [ ] build reproducibility notes

### Vita3K
- [ ] pin upstream
- [ ] replace desktop window/input/audio path
- [ ] consume PS5 Dynarmic executable code cache
- [ ] Vulkan renderer bring-up
- [ ] Vita firmware/user-data path policy
- [ ] first app/game boot

### Cemu
- [ ] identify wxWidgets/UI dependency boundary
- [ ] produce headless core build plan
- [ ] map PPC recompiler memory assumptions
- [ ] Vulkan bring-up
- [ ] controller mapping
- [ ] first Wii U title boot

### xemu
- [ ] isolate QEMU host/platform requirements
- [ ] evaluate current Vulkan renderer on PS5 RADV
- [ ] investigate RDNA2 performance issues upstream
- [ ] replace host window/audio/input
- [ ] BIOS/EEPROM/HDD user-data model

### Xenia
- [ ] audit x64 backend portability
- [ ] audit Vulkan path requirements
- [ ] map EDRAM/shader assumptions to PS5 RADV
- [ ] replace OS/platform services
- [ ] first XEX boot

## Phase 3 — Existing scene-port consolidation

- [ ] PPSSPP
- [ ] Mupen64Plus
- [ ] Azahar
- [ ] Dolphin
- [ ] Beetle PSX / Saturn
- [ ] DeSmuME
- [ ] MAME / VICE

For these, the goal is not to rewrite working public ports. The goal is to document, upstream where possible, and extract reusable PS5 platform lessons.

## Phase 4 — UX

Only after cores are stable:

- unified optional launcher
- per-system artwork
- per-game settings
- common save/config paths
- performance overlay
- update mechanism
- controller profiles
- network storage

The launcher must remain optional; each emulator should be runnable independently.
