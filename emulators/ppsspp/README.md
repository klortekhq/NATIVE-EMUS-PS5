# PPSSPP / PSP

## Status

Public PS5-specific PPSSPP work exists and should be studied before writing another port.

## Goals

- preserve PPSSPP JIT
- preserve Vulkan
- identify PS5 platform delta
- move generic JIT/memory/input/audio pieces into ps5rt
- avoid frontend lock-in

## Next milestone

Diff a current public PS5 PPSSPP fork against upstream PPSSPP and inventory reusable PS5 adaptations.
