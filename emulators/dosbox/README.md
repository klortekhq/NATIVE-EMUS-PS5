# DOS / IBM PC compatibles

Preferred base: DOSBox Staging.

Reference: https://github.com/dosbox-staging/dosbox-staging

## Why this base

It is actively maintained and already structured as a standalone emulator.

## Port direction

- keep DOSBox CPU/dynamic core architecture;
- replace host window/audio/input;
- map virtual drives through ps5rt::vfs;
- support keyboard/mouse properly;
- avoid Linux/Wine as the runtime architecture.

## Next milestone

Boot DOS shell natively on PS5 and run one software-rendered DOS title from /data.
