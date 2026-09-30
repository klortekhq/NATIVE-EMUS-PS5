# Architecture

## Objective

NATIVE-EMUS-PS5 is a family of **native PlayStation 5 emulator ports** sharing a small PS5 platform layer.

The architectural rule is simple:

```text
upstream emulator core
        |
        v
thin PS5 platform adaptation
        |
        v
native PS5 runtime / graphics / input / audio
```

Not:

```text
emulator -> RetroArch -> Linux -> compatibility layer -> PS5
```

RetroArch ports, Linux ports and other scene projects are valuable references, but they are not the required runtime architecture for this project.

## Host assumptions

- CPU: AMD Zen 2, x86-64
- SIMD: SSE4.1 / AVX2 available
- GPU: AMD RDNA2-class PS5 GPU
- Preferred graphics API: Vulkan through PS5 Mesa/RADV work
- Native system services: PS5 userland libraries exposed through public homebrew research / SDKs
- JIT: executable memory must use PS5-compatible mapping paths instead of assuming desktop mmap/mprotect semantics

## Shared runtime

The intended shared layer is kept intentionally small.

### ps5rt::memory

Responsibilities:

- flexible memory
- direct / pooled memory where needed
- executable JIT mappings
- alias mappings where required
- page-size and alignment normalization
- GPU-visible address-space constraints
- cache / instruction synchronization helpers

References already proving useful:

- Swordpdf/PS5SX2 JIT and mmap shims
- mihawk-99 PS5_Dynarmic executable code-cache work
- ps5-payload-dev SDK memory interfaces

### ps5rt::jit

Responsibilities:

- create executable code caches
- support RW/RX or RWX strategy required by the target
- expose cache flush hooks
- handle large per-core caches without exhausting the flexible-memory pool
- optional diagnostics for address ranges and fragmentation

This module must not contain emulator-specific recompilers.

### ps5rt::thread

Responsibilities:

- pthread/std::thread compatibility glue
- explicit stack sizing where desktop defaults are unsafe
- affinity / scheduling experiments
- per-thread diagnostics
- safe shutdown ordering

PS5SX2 has already demonstrated that default desktop assumptions about thread stack size can be wrong on PS5.

### ps5rt::tls

Responsibilities:

- support compiler-rt emulated TLS where required
- satisfy emulator-specific thread_local initialization where a clean generic path is not yet available
- avoid carrying ad-hoc assembly stubs into every emulator port

### ps5rt::vulkan

Preferred design:

```text
emulator Vulkan backend
        |
        v
Vulkan loader / platform glue
        |
        v
Mesa / RADV PS5 winsys
        |
        v
PS5 GPU
```

The project should reuse the strongest public PS5 RADV/Mesa path available while keeping emulator-specific renderer code upstream-shaped.

Important concerns:

- command submission pressure
- readback stalls
- shader cache persistence
- PS5 GPU-visible address windows
- threading of Vulkan command recording
- formats unsupported by the PS5 path
- synchronization hazards exposed by console-specific scheduling

### ps5rt::audio

Target: direct PS5 audio output.

Expected responsibilities:

- libSceAudioOut initialization
- stereo / multichannel format negotiation
- ring-buffer or blocking-grain backend
- pause / resume
- emulator time-stretch integration

### ps5rt::input

Target devices:

- DualSense
- multiple signed-in users/controllers
- optional USB keyboard/mouse
- rumble
- touchpad shortcuts only when useful to a frontend

The emulator core should receive ordinary logical inputs; PS5-specific polling belongs here.

### ps5rt::vfs

Responsibilities:

- /data
- internal user data
- M.2 / external storage paths
- USB discovery
- save/config separation from binaries
- optional future SMB/network paths

No emulator should invent its own unrelated storage policy unless upstream requires it.

### ps5rt::app

Responsibilities:

- startup/shutdown
- privilege request integration where required
- logging
- crash/signal reporting
- user/service initialization
- launch metadata
- suspend/resume research

### ps5rt::toolchain

Tracked toolchains:

- prospero-clang based PS5 SDK toolchains
- LLVM 18 known-good baseline for several current scene projects
- LLVM 23 support now present in ps5-payload-dev, to be evaluated carefully
- PS5-specific LLVM ABI fixes from mihawk-99/PS5_LLVM

Compiler upgrades should be deliberate and reproducible, not automatic.

## Per-emulator pattern

Each emulator directory should eventually contain:

```text
emulators/<name>/
  README.md
  UPSTREAM.md
  PORTING.md
  patches/
  ps5/
  tests/
```

The preferred port strategy is:

1. pin a known upstream revision;
2. identify desktop/platform dependencies;
3. keep CPU core/JIT intact when host-compatible;
4. route executable memory through ps5rt;
5. retain upstream Vulkan renderer where practical;
6. replace SDL/Qt/wx/cubeb platform services only where necessary;
7. boot headless before building a rich frontend;
8. add native PS5 UX after the core path is stable.

## Anti-Frankenstein rule

A port is considered architecturally clean when the emulated machine is translated directly by its intended core into PS5-host execution/rendering.

Examples:

- PCSX2 x86-64 recompiler on PS5: good.
- Dynarmic ARM -> x86-64 code on PS5: good.
- RPCS3 LLVM PPU/SPU -> x86-64 on PS5: good.
- Xenia x64 backend on PS5: good.
- Running a Linux build inside a VM merely to call it a PS5 port: not the project target.
- Nesting an emulator inside another emulator frontend as a hard dependency: not the project target.

## Licensing

The shared runtime must use code only where licensing is compatible and attribution obligations are understood.

Where a useful implementation is GPL-only but the shared runtime needs a different licensing boundary, prefer a clean-room reimplementation from documented behavior/API research rather than copying code blindly.
