# Dolphin / GameCube · Wii

## Status

Existing PS5 scene work demonstrates that Dolphin JIT + Vulkan is feasible.

## Project goal

Keep Dolphin's JIT and Vulkan path native and make the PS5 host layer standalone.

## Critical areas

- executable JIT memory
- exception/fault handling
- MMU/fastmem behavior
- Vulkan
- audio
- controller mapping
- Wii-specific motion/IR UX

## Next milestone

Extract the PS5-specific Dolphin build/JIT/platform requirements from existing public work and define a standalone target.
