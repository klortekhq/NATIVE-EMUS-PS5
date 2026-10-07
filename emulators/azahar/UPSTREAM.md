# Azahar upstream pin

Canonical repository: https://github.com/azahar-emu/azahar

## Current PS5 bring-up pin

- Azahar: `662d412123305a9f4be94dd3dc73ddf91a18c55e`
- Dynarmic submodule: `azahar-emu/dynarmic@e77b1ba0b7da7cbe93021b01a663acfe7c4dd516`
- guest CPU frontend: A32
- host code generator: x86-64 / Xbyak
- project license: GPL-2.0 text in `license.txt`; bundled third-party licenses remain separate

Verified: 2026-10-01.

The Dynarmic revision is read from the canonical Azahar gitlink, not selected
independently. This avoids mixing a newer/older Dynarmic ABI into the first PS5
CPU gate.

## PS5 memory implication

This Dynarmic reserves a large code range and, on Windows, commits it
incrementally. Unix relies on virtual-memory demand paging. A PS5 port must not
replace that behavior with one eager direct-memory allocation.

The PS5 gate therefore preserves the same high-level contract using
`ps5rt_sparse_arena`:

1. reserve the complete stable virtual range;
2. commit direct-memory chunks as the code cache grows;
3. transition the committed prefix between RW and RX;
4. release the arena as one allocator lifetime.

Vita3K intentionally remains on its own pinned Dynarmic revision. The shared
piece is the `ps5rt` executable-memory contract, not a forced common Dynarmic
source tree.
