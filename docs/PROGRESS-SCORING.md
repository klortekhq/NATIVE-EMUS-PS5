# Emulator progress scoring

Every emulator README in NATIVE-EMUS-PS5 should expose a visible percentage as that emulator is actively brought into the native PS5 tree.

The number is evidence-based, not a subjective "looks almost done" estimate.

## Default 100-point rubric

| Gate | Weight |
|---|---:|
| Pinned upstream/donor + provenance/licensing mapped | 10% |
| Reproducible PS5 engine cross-build | 15% |
| Native CPU JIT/dynarec/recompiler when upstream provides one | 25% |
| Native graphics path | 15% |
| Native audio/input | 10% |
| VFS/media/saves | 10% |
| Native title/package shell | 5% |
| Physical PS5 boot/playability validation | 10% |

Total: **100%**

## Recompiler rule

For an emulator with an upstream JIT/dynarec/recompiler, the CPU gate cannot receive full credit for an interpreter-only PS5 build.

Interpreter execution may be used for diagnosis/bring-up, but it is not considered the final architecture.

Examples:

- PS1: Lightrec + GNU Lightning x86-64.
- PS2: PCSX2 EE/IOP/VU x86-64 recompilers.
- PS3: RPCS3 PPU/SPU LLVM paths.
- PSP: PPSSPP x86-64 JIT.
- Dreamcast: Flycast rec-x64.
- N64: R4300 new_dynarec + RSP JIT.
- Vita/3DS/Switch family: Dynarmic x86-64.
- GameCube/Wii: Dolphin Jit64.
- Wii U: Cemu PPC recompiler.
- Xbox 360: Xenia x64 backend.

## Small interpreter-designed systems

If the selected upstream core has no meaningful dynamic recompiler, the 25% CPU gate is reassigned to core correctness/build integration rather than inventing a JIT that upstream does not need.

## Evidence levels

A gate may receive partial credit while implementation exists but is not fully validated. Full credit requires reproducible evidence appropriate to the gate:

- CI archive/ELF output for cross-builds;
- object/symbol verification for JIT engines;
- console logs/screenshots for hardware boot;
- actual input/audio/rendering evidence for playability.

## Updating percentages

When a gate changes, update both:

1. the emulator's `emulators/<name>/README.md`;
2. the canonical `systems/<system>/README.md` where applicable.

Shared engines may have multiple system pages but one engine implementation score.
