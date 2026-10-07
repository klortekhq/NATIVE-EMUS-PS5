#include "../native/display_surface.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>

namespace {

VkInstance fake_instance() {
  return reinterpret_cast<VkInstance>(static_cast<std::uintptr_t>(0x1000));
}
VkPhysicalDevice fake_gpu() {
  return reinterpret_cast<VkPhysicalDevice>(static_cast<std::uintptr_t>(0x2000));
}
VkDisplayKHR fake_display() {
  return reinterpret_cast<VkDisplayKHR>(static_cast<std::uintptr_t>(0x3000));
}
VkDisplayModeKHR fake_mode() {
  return reinterpret_cast<VkDisplayModeKHR>(static_cast<std::uintptr_t>(0x4000));
}
VkSurfaceKHR fake_surface() {
  return reinterpret_cast<VkSurfaceKHR>(static_cast<std::uintptr_t>(0x5000));
}

int display_queries{};
int mode_queries{};
int surface_creates{};

VKAPI_ATTR VkResult VKAPI_CALL fake_vkGetPhysicalDeviceDisplayPropertiesKHR(
    VkPhysicalDevice gpu,
    std::uint32_t* count,
    VkDisplayPropertiesKHR* out) {
  assert(gpu == fake_gpu());
  ++display_queries;
  if (!out) {
    *count = 1;
    return VK_SUCCESS;
  }
  assert(*count >= 1);
  out[0] = {};
  out[0].display = fake_display();
  out[0].physicalResolution = {3840, 2160};
  *count = 1;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL fake_vkGetDisplayModePropertiesKHR(
    VkPhysicalDevice gpu,
    VkDisplayKHR display,
    std::uint32_t* count,
    VkDisplayModePropertiesKHR* out) {
  assert(gpu == fake_gpu());
  assert(display == fake_display());
  ++mode_queries;
  if (!out) {
    *count = 1;
    return VK_SUCCESS;
  }
  assert(*count >= 1);
  out[0] = {};
  out[0].displayMode = fake_mode();
  out[0].parameters.visibleRegion = {3840, 2160};
  out[0].parameters.refreshRate = 59940;
  *count = 1;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL fake_vkCreateDisplayPlaneSurfaceKHR(
    VkInstance instance,
    const VkDisplaySurfaceCreateInfoKHR* info,
    const VkAllocationCallbacks*,
    VkSurfaceKHR* out) {
  assert(instance == fake_instance());
  assert(info);
  assert(info->displayMode == fake_mode());
  assert(info->planeIndex == 0);
  assert(info->planeStackIndex == 0);
  assert(info->transform == VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR);
  assert(info->alphaMode == VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR);
  assert(info->imageExtent.width == 3840);
  assert(info->imageExtent.height == 2160);
  ++surface_creates;
  *out = fake_surface();
  return VK_SUCCESS;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL fake_vkGetInstanceProcAddr(
    VkInstance, const char* name) {
  if (std::strcmp(name, "vkGetPhysicalDeviceDisplayPropertiesKHR") == 0)
    return reinterpret_cast<PFN_vkVoidFunction>(
        fake_vkGetPhysicalDeviceDisplayPropertiesKHR);
  if (std::strcmp(name, "vkGetDisplayModePropertiesKHR") == 0)
    return reinterpret_cast<PFN_vkVoidFunction>(
        fake_vkGetDisplayModePropertiesKHR);
  if (std::strcmp(name, "vkCreateDisplayPlaneSurfaceKHR") == 0)
    return reinterpret_cast<PFN_vkVoidFunction>(
        fake_vkCreateDisplayPlaneSurfaceKHR);
  return nullptr;
}

} // namespace

int main() {
  const VkSurfaceKHR surface =
      native_emus::ps1::create_ps5_display_surface(
          fake_instance(), fake_gpu(), fake_vkGetInstanceProcAddr);
  assert(surface == fake_surface());
  assert(display_queries == 2);
  assert(mode_queries == 2);
  assert(surface_creates == 1);

  assert(native_emus::ps1::create_ps5_display_surface(
             VK_NULL_HANDLE, fake_gpu(), fake_vkGetInstanceProcAddr) ==
         VK_NULL_HANDLE);
  return 0;
}
