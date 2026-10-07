# Dynarmic PS5 x86-64 JIT dependency

## Progress: **50%**

> Native PS5 port progress, scored from reproducible engineering evidence — not upstream emulator compatibility. See [progress scoring](../../docs/PROGRESS-SCORING.md).

Dynarmic is a shared dependency for several modern ARM-family emulator ports.

## PS5 donor pin

- Donor: https://github.com/mihawk-99/PS5_Dynarmic
- Commit: `1df2a07e9a86afef73d511a931f40bedb40b30c4`
- Date: 2026-09-28

That commit moves Xbyak's code cache to PS5 executable memory because a default 128 MiB code cache per emulated core can exhaust the title's flexible-memory pool.

## NATIVE-EMUS-PS5 integration

`tools/ps5/apply_dynarmic_ps5rt.py` keeps Mihawk's PS5 allocator behavior but retargets it to:

- `ps5rt_exec_allocate()`
- `ps5rt_exec_release()`

This prevents Vita3K, Azahar and Switch-family ports from carrying separate PS5 JIT allocators.

## CPU policy

Dynarmic's **x86-64 JIT is mandatory** on PS5 for final builds. Interpreter paths are not the target architecture.
