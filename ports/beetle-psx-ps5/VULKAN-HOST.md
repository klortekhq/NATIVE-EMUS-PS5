# PS1 Vulkan host gate

This document records the next PS1 gate after the Lightrec CPU/JIT engine.

## Objective

Keep Beetle PSX HW's existing Vulkan RHI and replace only the frontend-owned
hardware-context negotiation with a native PS5 provider.

The renderer already exposes a narrow boundary:

- `rhi_vulkan_set_environment()`
- `rhi_vulkan_set_video_refresh()`
- `RETRO_ENVIRONMENT_SET_HW_RENDER`
- `RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE`
- `RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE`

Its device-creation callback receives:

- `VkInstance`
- `VkPhysicalDevice`
- `VkSurfaceKHR`
- `vkGetInstanceProcAddr`
- required device extensions/layers/features

and returns the selected device, presentation queue and queue-family indices.

## Native PS5 architecture

```text
Beetle PSX HW RHI
       |
       | minimal libretro HW-context ABI
       v
PS1 native Vulkan host
       |
       +-- linked vkGetInstanceProcAddr
       +-- PS5 Vulkan/RADV instance
       +-- physical device
       +-- KHR_display / PS5 WSI
       +-- presentation queue
       |
       v
PS5_Vulkan / Mesa RADV
       |
       v
PS5 GPU
```

There is no RetroArch executable in this chain.

## Important donor evidence

The public PS5 ecosystem already demonstrates that the Vulkan driver should be
**linked into the title**, rather than dlopened at runtime. The PS5_RetroArch
port documents that the console title cannot load the driver as a conventional
shared object and links the driver/runtime archives directly.

NATIVE-EMUS-PS5 should follow the same platform fact while keeping its own
standalone host.

## RADV threaded recording

Do **not** enable `RADV_THREADED_RECORDING=1` by default for PS1 yet.

The 2026-09-30 PS5 Mesa work makes threaded recording available and fixes
several CTS-discovered lifetime/reset problems, but PS5_Vulkan still treats it
as an opt-in gated path. PS1 will first validate the normal RADV path. Threaded
recording becomes a measured performance experiment afterwards.

## Renderer completion criteria

The 15% graphics gate is complete only when all of these are true:

1. Beetle's Vulkan RHI initializes without RetroArch.
2. A PS5 Vulkan/RADV device and presentation path are supplied by the native host.
3. Frame submission reaches the PS5 display.
4. Context reset/destroy survives repeated content reload.
5. At least one legal PS1 test program or BIOS screen renders on physical PS5.
6. No software-renderer fallback is being counted as Vulkan success.

## Immediate implementation order

1. model the minimal hardware-context ABI in `corehost`;
2. provide a native PS5 Vulkan context object;
3. feed the interface to Beetle's existing environment callback;
4. preserve the donor's `libretro_create_device()` and RHI internals;
5. validate on normal RADV;
6. only then benchmark optional threaded recording.
