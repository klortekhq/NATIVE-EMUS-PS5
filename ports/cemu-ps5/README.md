# Cemu native PS5 x64 recompiler gate

Pinned upstream: `cemu-project/Cemu@4e3c824faa00f6b85782db019f20f29f063f3a2a`.

The first gate intentionally isolates the Wii U Espresso CPU engine:

```text
Espresso PowerPC guest
  -> Cemu PPC IML
  -> optimizer / register allocator
  -> BackendX64 AVX/BMI/FPU
  -> PS5 Zen 2
```

The archive contains the recompiler frontend, IML passes and all current
BackendX64 translation units. It does not build wxWidgets, desktop UI or the
GPU renderer.

Compiler tuning:

- Zen 2
- SSE4.1
- AVX2
- BMI/BMI2
- no VZEROUPPER workaround

After this archive is stable, executable-memory allocation is retargeted to
`ps5rt`, followed by Cemu's Vulkan renderer on the shared PS5 RADV path.

The interpreter is retained only as upstream fallback/diagnostic machinery,
not as the final host CPU engine.
