# Dynarmic PS5 shared JIT gate

This is not a console emulator by itself. It is the shared **ARM -> x86-64 JIT engine gate** used by multiple native PS5 ports.

Consumers:

- Vita3K / PlayStation Vita
- Azahar / Nintendo 3DS
- Switch-family work such as ProsperoEden-derived research

## CPU policy

```text
ARM guest
  -> Dynarmic A32/A64 frontend
  -> x86-64 backend / Xbyak
  -> ps5rt executable memory
  -> PS5 Zen 2
```

The build gate rejects an archive that lacks the x64 A32/A64 JIT objects.

## Pin

`mihawk-99/PS5_Dynarmic@1df2a07e9a86afef73d511a931f40bedb40b30c4`

The PS5 allocator delta is transformed from the donor-specific `ps5_exec_*` API to the common `ps5rt_exec_*` ABI before compilation.


## CI-verified state

**PS5 cross-build: green.**

Workflow run `36760157664` successfully produced and uploaded `libdynarmic_ps5.a` with the A32 and A64 x86-64 JIT objects present. This moves the shared ARM backend from transform-only research to a reproducible PS5 engine build.

Hardware execution/fastmem validation on a jailbroken PS5 is still a separate gate.
