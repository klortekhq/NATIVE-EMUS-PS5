# Dolphin native PS5 engine gate

Pinned donor/source: `libretro/dolphin@c6630001e05780b7c03e661a4a539b59ef716ebc`.

This workspace builds Dolphin as a **static engine boundary only**. RetroArch is
not a runtime dependency: `LIBRETRO=ON` is used to disable desktop frontend
dependencies while preserving Dolphin's own CPU cores.

Required final CPU paths:

- PowerPC -> Dolphin **Jit64** -> PS5 x86-64 Zen 2
- DSP -> Dolphin **x64 DSP JIT** -> PS5 x86-64 Zen 2
- fastmem -> PS5 direct-memory mappings through `ps5rt`
- executable code -> `ps5rt_exec_allocate`

The interpreter remains upstream fallback for individual unsupported
instructions; it is not the selected host CPU engine.

Current gate deliberately disables graphics so CPU/JIT/fastmem can be proven
first. Vulkan/RADV is the next layer after the static core archive is stable.
