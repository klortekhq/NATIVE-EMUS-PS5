#pragma once

#include <vulkan/vulkan.h>

namespace native_emus::ps1 {

// Creates the console display surface through VK_KHR_display. On the PS5 RADV
// port this standard Vulkan extension is backed by VideoOut.
//
// The implementation deliberately mirrors the public PS5_Vulkan RADV smoke
// path: one physical display, first advertised display mode, display plane 0,
// identity transform and opaque alpha.
VkSurfaceKHR create_ps5_display_surface(
    VkInstance instance,
    VkPhysicalDevice gpu,
    PFN_vkGetInstanceProcAddr get_instance_proc_addr) noexcept;

} // namespace native_emus::ps1
