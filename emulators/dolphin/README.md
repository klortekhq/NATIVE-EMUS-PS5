# Dolphin / GameCube · Wii

## Status

Dolphin remains the generic GameCube/Wii emulator target. Existing PS5 scene work demonstrates that Dolphin's x86-64 JIT and Vulkan path are feasible, while Phi1ow's `mkwii-ps5` provides additional native PS5 platform evidence through a **static Wii game recompilation**.

These are complementary, not interchangeable:

```text
Dolphin
  PPC guest at runtime
    -> Dolphin x86-64 JIT
    -> fastmem / MMU / exception machinery
    -> Vulkan -> PS5 RADV
```

```text
mkwii-ps5 / WiiCompiled
  one specific Wii title
    -> ahead-of-time/static recompiled host code
    -> native PS5 runtime
    -> AGC
```

The project goal is still the first architecture: a **generic native Dolphin port**.

## CPU policy

Final PS5 build must retain Dolphin's x86-64 PPC JIT.

Interpreter mode is diagnostic/fallback only.

## Useful platform findings from mkwii-ps5

The public `Phi1ow/mkwii-ps5` source gives us PS5-specific lessons that can be applied independently around Dolphin:

- reserve guest virtual ranges with fixed + no-overwrite semantics for the initial claim;
- use cached direct memory for CPU guest RAM and map fixed aliases into the owned guest arena;
- distinguish general cached direct memory from CPU/GPU-shared direct memory;
- restore reservations when aliases are removed instead of leaving allocator races;
- 16 KiB PS5 page granularity must be handled explicitly;
- native SysV x86-64 fiber/context switching is viable;
- native 4-user pad lifecycle and vibration are viable;
- AudioOut and VideoOut can be driven without a desktop host layer;
- AGC is a viable native renderer for custom ports, but Dolphin's first target remains Vulkan/RADV because it preserves Dolphin's upstream renderer architecture.

## Critical Dolphin areas

- x86-64 JIT executable memory
- fastmem address-space reservation and aliases
- page-fault / exception context on PS5
- MMU behavior
- Vulkan/RADV surface/device integration
- shader cache
- AudioOut
- DualSense / GameCube / Wii input
- Wii-specific motion and IR UX
- filesystem, NAND and save paths
- native title lifecycle

## Current shared-runtime work already benefiting Dolphin

- `ps5rt_exec_*`: executable JIT allocation
- `ps5rt_shm_*`: direct-memory-backed shared mappings
- `ps5rt_vrange_*`: virtual address reservations
- `ps5rt_vmem_*`: commit/decommit/protection inside reservations
- initial reservations use fixed + no-overwrite semantics
- CPU cached memory type and CPU/GPU-shared memory type are separated
- multi-user DualSense + rumble support in `ps5rt::input`

## Next milestone

Build a Dolphin-specific PS5 platform probe that validates:

1. the exact fastmem reservation size/layout Dolphin requests;
2. mirrored guest RAM aliases over `ps5rt_shm`;
3. Dolphin x86-64 JIT code-cache allocation;
4. PS5 signal/ucontext register extraction for JIT fault recovery;
5. Vulkan instance/device creation over the common PS5 RADV stack.

Only after those five gates pass should we attempt GameCube IPL/Wii boot.
