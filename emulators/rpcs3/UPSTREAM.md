# RPCS3 upstream audit

Last reviewed: 2026-10-01

## Active PS5 CPU gate pin

- RPCS3: `RPCS3/rpcs3@83b1e072990e839903c67401d1a6d7d1910a5824`
- LLVM submodule: `llvm/llvm-project@ca7933e47d3a3451d81e72ac174dcb5aa28b59d1`
- AsmJit submodule: `asmjit/asmjit@416f7356967c1f66784dc1580fe157f9406d8bff`

This exact stack is the one for which the PPU LLVM + SPU LLVM/AsmJit PS5
cross-build evidence and sparse-memory follow-up are recorded.

## Newer canonical RPCS3 revision

Canonical RPCS3 advanced during the 2026-10-01 audit to:

`81cbf7ba3d872faf4b13f679db4878cf44a93bcc`

It is one commit ahead of the active gate pin. The compared delta changes only:

- `rpcs3/Emu/Cell/lv2/sys_net.cpp`
- `rpcs3/Emu/NP/np_requests.cpp`
- `rpcs3/Emu/NP/signaling_handler.cpp`

The commit fixes sendmsg sleep/timeout behavior, NP TUS info bounds/copy
handling and signaling packet retirement.

No PPU/SPU recompiler, LLVM, AsmJit, JIT allocator, virtual-memory or renderer
source changed in this delta.

## Rebase decision

Do **not** move the active CPU/memory evidence pin merely because canonical
`master` advanced. The current PS5 gate remains pinned at `83b1e072...`
until the 2 GiB sparse-JIT arena has been executed on physical PS5.

After that hardware gate, rebase deliberately onto the then-current canonical
RPCS3 revision and rerun the complete source/ABI/PPU/SPU engine gate. This
avoids mixing an unrelated networking/NP update into a memory-validation
checkpoint while still recording the upstream bug fixes for the next rebase.

Historical Mihawk PS5 RPCS3/LLVM/Mesa revisions remain provenance references
only and are not substituted for canonical RPCS3.
