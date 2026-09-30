#include "display_surface.hpp"

#include <vector>

namespace native_emus::ps1 {
namespace {

template <typename T>
T load_instance(
    PFN_vkGetInstanceProcAddr get_proc,
    VkInstance instance,
    const char* name) noexcept {
  return get_proc
      ? reinterpret_cast<T>(get_proc(instance, name))
      : nullptr;
}

} // namespace

VkSurfaceKHR create_ps5_display_surface(
    VkInstance instance,
    VkPhysicalDevice gpu,
    PFN_vkGetInstanceProcAddr get_instance_proc_addr) noexcept {
  if (instance == VK_NULL_HANDLE || gpu == VK_NULL_HANDLE ||
      !get_instance_proc_addr)
    return VK_NULL_HANDLE;

  const auto get_displays =
      load_instance<PFN_vkGetPhysicalDeviceDisplayPropertiesKHR>(
          get_instance_proc_addr, instance,
          "vkGetPhysicalDeviceDisplayPropertiesKHR");
  const auto get_modes =
      load_instance<PFN_vkGetDisplayModePropertiesKHR>(
          get_instance_proc_addr, instance,
          "vkGetDisplayModePropertiesKHR");
  const auto create_surface =
      load_instance<PFN_vkCreateDisplayPlaneSurfaceKHR>(
          get_instance_proc_addr, instance,
          "vkCreateDisplayPlaneSurfaceKHR");

  if (!get_displays || !get_modes || !create_surface)
    return VK_NULL_HANDLE;

  std::uint32_t display_count = 0;
  if (get_displays(gpu, &display_count, nullptr) != VK_SUCCESS ||
      display_count == 0)
    return VK_NULL_HANDLE;

  std::vector<VkDisplayPropertiesKHR> displays(display_count);
  if (get_displays(gpu, &display_count, displays.data()) != VK_SUCCESS ||
      display_count == 0)
    return VK_NULL_HANDLE;

  // PS5_Vulkan exposes the console output as a single VK_KHR_display display.
  // Keep enumeration generic so a host-model test or later driver revision
  // with more than one reported display remains well-defined.
  const VkDisplayKHR display = displays.front().display;
  if (display == VK_NULL_HANDLE)
    return VK_NULL_HANDLE;

  std::uint32_t mode_count = 0;
  if (get_modes(gpu, display, &mode_count, nullptr) != VK_SUCCESS ||
      mode_count == 0)
    return VK_NULL_HANDLE;

  std::vector<VkDisplayModePropertiesKHR> modes(mode_count);
  if (get_modes(gpu, display, &mode_count, modes.data()) != VK_SUCCESS ||
      mode_count == 0)
    return VK_NULL_HANDLE;

  const auto& mode = modes.front();
  if (mode.displayMode == VK_NULL_HANDLE ||
      mode.parameters.visibleRegion.width == 0 ||
      mode.parameters.visibleRegion.height == 0)
    return VK_NULL_HANDLE;

  VkDisplaySurfaceCreateInfoKHR info{};
  info.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
  info.displayMode = mode.displayMode;
  info.planeIndex = 0;
  info.planeStackIndex = 0;
  info.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  info.globalAlpha = 1.0f;
  info.alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
  info.imageExtent = mode.parameters.visibleRegion;

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  return create_surface(instance, &info, nullptr, &surface) == VK_SUCCESS
      ? surface
      : VK_NULL_HANDLE;
}

} // namespace native_emus::ps1
