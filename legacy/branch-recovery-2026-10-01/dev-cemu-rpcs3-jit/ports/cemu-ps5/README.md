# Cemu PS5 — PPC x64 recompiler gate

Pinned upstream:

- `cemu-project/Cemu`
- commit `4e3c824faa00f6b85782db019f20f29f063f3a2a`

The first PS5 stage deliberately targets the **existing x86-64 PPC
recompiler**, not the interpreter and not the wxWidgets UI.

```text
Wii U Espresso PPC
    -> Cemu PPC IML
    -> BackendX64
    -> ps5rt_exec_allocate()
    -> PS5 Zen 2
```

## First transform

Cemu's x64 backend currently allocates 4 MiB-or-larger RWX code-cache blocks
through:

`MemMapper::AllocateMemory(..., P_RWX)`

On PS5, only that executable-memory boundary is replaced with
`ps5rt_exec_allocate()`. IML generation, optimization, register allocation,
x64 emission and the rest of the emulator stay upstream-shaped.

## Deliberately not done in this gate

- no wxWidgets port;
- no interpreter substitution;
- no OpenGL fallback;
- no fake Vulkan surface;
- no renderer changes yet.

After the x64 engine is independently cross-buildable, Latte Vulkan is the
next layer and should target the shared PS5 RADV path.
