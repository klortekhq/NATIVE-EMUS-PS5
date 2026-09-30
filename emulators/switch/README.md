# Switch-family emulator research

## Progress: **30%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

## Scope

This directory tracks architecture lessons from public native PS5 Switch-emulator work such as ProsperoEden.

It is not a statement that a specific upstream or fork is redistributed here.

## Useful lessons

- Dynarmic on PS5
- native Vulkan/RADV
- emulated TLS
- x86-64 context switching
- filesystem on internal/M.2/USB
- DualSense/user mapping
- clean native PS5 title packaging

## Project rule

Treat public implementations as references and integrate only source that is legally and license-compatible to redistribute.

## Next milestone

Classify ProsperoEden platform work into generic ps5rt modules vs emulator-specific code.


## Shared Dynarmic PS5 gate

- [x] PS5 Xbyak executable-code allocator identified in Mihawk's fork
- [x] allocator retargeted to shared `ps5rt_exec_*`
- [x] final CPU policy: Dynarmic x86-64 JIT, not full interpretation
- [x] Shared Dynarmic A32+A64 x86-64 JIT archive cross-builds on PS5\n- [ ] Switch-family core cross-compile against the shared allocator
- [ ] physical PS5 JIT/fastmem validation
