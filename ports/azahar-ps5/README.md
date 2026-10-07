# Azahar Dynarmic PS5 engine gate

This gate builds the **exact Dynarmic revision pinned by canonical Azahar** as
an ARMv7/A32 -> x86-64 JIT for PlayStation 5.

Pinned stack:

- Azahar: `azahar-emu/azahar@662d412123305a9f4be94dd3dc73ddf91a18c55e`
- Dynarmic: `azahar-emu/dynarmic@e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`

## Why this is separate from Vita3K

Vita3K and Azahar pin different Dynarmic revisions. We do not force both
emulators onto one moving fork.

The common layer is `ps5rt`:

~~~text
3DS ARMv7
  -> Azahar Dynarmic A32
  -> Xbyak x86-64
  -> ps5rt sparse fixed-address arena
  -> incremental PS5 direct-memory commits
  -> RW / RX W^X transitions
  -> PS5 Zen 2
~~~

Azahar's current Dynarmic already models a large stable code reservation and
incremental commits on Windows. The PS5 transform preserves that behavior
instead of replacing it with one eager physical allocation.

## Reproducible evidence

Workflow `36868097926`, job
`ps5-cross-build-azahar-dynarmic-engine`, completed successfully with the
public Prospero toolchain.

Artifact:

- name: `native-azahar-dynarmic-a32-x64-ps5-engine`
- artifact ID: `11165471584`
- ZIP SHA-256:
  `bec837039aabc00c269c3094c67b0843e06367cecfdc7498714c34fe2b198def`

The build emits `libazahar_dynarmic_ps5.a` and verifies that the transformed
x64 backend contains the sparse reserve, incremental commit and protection
bridge.

## What this proves

It proves that the pinned Azahar Dynarmic A32 x64 engine and the PS5 sparse
memory adaptation **cross-compile** together.

It does not yet prove:

- emitted x86-64 execution on physical PS5;
- guest fastmem behavior;
- full `citra_core` / Azahar core linking;
- Vulkan;
- native audio/input/VFS;
- game boot.

Those remain separate evidence gates.
