# Azahar / Nintendo 3DS

## Progress: **30%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

## Canonical CPU stack

The current bring-up is pinned to:

- `azahar-emu/azahar@662d412123305a9f4be94dd3dc73ddf91a18c55e`
- its exact Dynarmic gitlink:
  `azahar-emu/dynarmic@e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`

Azahar uses the A32 Dynarmic frontend and x86-64/Xbyak host backend.

## PS5 CPU gate

The canonical Dynarmic revision now cross-builds with Prospero after a
deterministic PS5-only allocator transformation:

- stable sparse virtual reservation through `ps5rt_sparse_arena`;
- incremental direct-memory commits as the code cache grows;
- RW/RX W^X transitions across multiple committed chunks;
- no interpreter substituted for the x86-64 JIT.

Workflow `36868097926` is green. Artifact
`native-azahar-dynarmic-a32-x64-ps5-engine` is ID `11165471584`, with ZIP
SHA-256
`bec837039aabc00c269c3094c67b0843e06367cecfdc7498714c34fe2b198def`.

Vita3K remains on its own pinned Dynarmic revision. The projects share the
`ps5rt` executable-memory contract, not a forced common source tree.

## Goals

- native Dynarmic x86-64 execution;
- Vulkan renderer;
- PS5 input/audio/VFS;
- standalone frontend path.

## Next gates

- [x] canonical Azahar upstream pinned;
- [x] exact Azahar Dynarmic submodule pinned;
- [x] A32 x86-64 Dynarmic archive cross-builds on PS5;
- [x] sparse reserve / incremental commit / W^X allocator bridge compiles;
- [ ] execute JIT/sparse-memory path on physical PS5;
- [ ] cross-compile Azahar core against the proven CPU engine;
- [ ] isolate headless/native frontend path;
- [ ] Vulkan bring-up;
- [ ] native AudioOut / DualSense / VFS;
- [ ] first 3DS guest boot.

Progress remains **30%** until runtime behavior or a larger Azahar core boundary
is proven; an engine cross-build alone is not counted as guest execution.
