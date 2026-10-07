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

## Vulkan compile gate

`build_vulkan_ps5.sh` enables Dolphin's own Vulkan backend and cross-builds
the `videovulkan` archive with the same pinned source and public PS5 toolchain
used by the Jit64 gate.

This deliberately stops before final RADV linkage. A green result proves that
the Dolphin Vulkan backend source, including its loader/context/swap-chain
objects, cross-compiles for the PS5 target. It does **not** prove that the
desktop dynamic Vulkan loader is suitable on PS5, that `VK_KHR_display`
presentation works, or that a frame has been shown on hardware.

The next graphics gate is therefore explicit: replace the desktop loader path
with the pinned static RADV entry-point contract, add the PS5
`VK_KHR_display` surface path, final-link a standalone title, then validate
presentation on physical hardware.
