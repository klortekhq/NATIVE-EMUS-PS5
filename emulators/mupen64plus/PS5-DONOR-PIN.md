# Mupen64Plus PS5 donor pin

- donor: `mihawk-99/PS5_Mupen64Plus`
- commit: `98ec1019d191fc3c0e1c2d031f2574e95bd8d30d`
- date: 2026-09-28

This commit is important because **both executable engines** are moved into PS5 executable memory:

1. Mupen64Plus new dynarec — 32 MiB cache.
2. ParaLLEl-RSP JIT — 64 MiB blocks.

The NATIVE-EMUS-PS5 transform preserves that behavior and replaces the donor's
private platform allocator with the shared `ps5rt_exec_*` ABI.

## Final CPU/RSP policy

- CPU: x86-64 new dynarec.
- RSP: ParaLLEl-RSP JIT.
- RDP: ParaLLEl-RDP / Vulkan path preferred.

Pure interpreter CPU/RSP modes are diagnostic fallbacks, not the final PS5 architecture.
