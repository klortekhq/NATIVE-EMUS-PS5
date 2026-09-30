#include "vulkan_environment.hpp"

#include <cstring>

namespace native_emus::ps1 {

bool VulkanEnvironment::environment(unsigned cmd, void* data) noexcept {
  switch (cmd) {
    case RETRO_ENVIRONMENT_SET_HW_RENDER: {
      if (!data) return false;
      auto* requested = static_cast<retro_hw_render_callback*>(data);
      if (requested->context_type != RETRO_HW_CONTEXT_VULKAN)
        return false;

      // A fresh HW request invalidates any previously published interface.
      context_destroy();
      hw_ = *requested;
      render_requested_ = true;
      negotiation_ready_ = false;
      interface_ready_ = false;

      // Vulkan cores use the API-specific render interface. These legacy
      // callbacks are not the presentation path, but leave their ownership
      // explicit instead of carrying stale frontend pointers.
      requested->get_current_framebuffer = nullptr;
      requested->get_proc_address = nullptr;
      hw_.get_current_framebuffer = nullptr;
      hw_.get_proc_address = nullptr;
      return true;
    }

    case RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE: {
      if (!render_requested_ || !data) return false;
      const auto* base =
          static_cast<const retro_hw_render_context_negotiation_interface*>(data);
      if (base->interface_type !=
          RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN)
        return false;

      const auto* vk =
          static_cast<const retro_hw_render_context_negotiation_interface_vulkan*>(
              data);
      if (vk->interface_version == 0 ||
          vk->interface_version >
              RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN_VERSION)
        return false;

      negotiation_ = *vk;
      negotiation_ready_ = true;
      return true;
    }

    case RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE: {
      if (!data || !interface_ready_ ||
          (!reset_in_progress_ && !context_live_))
        return false;

      auto** out = static_cast<const retro_hw_render_interface**>(data);
      *out = reinterpret_cast<const retro_hw_render_interface*>(&interface_);
      return true;
    }

    default:
      return false;
  }
}

bool VulkanEnvironment::publish_interface(
    const retro_hw_render_interface_vulkan& iface) noexcept {
  if (!render_requested_) return false;
  if (iface.interface_type != RETRO_HW_RENDER_INTERFACE_VULKAN)
    return false;
  if (iface.interface_version == 0 ||
      iface.interface_version > RETRO_HW_RENDER_INTERFACE_VULKAN_VERSION)
    return false;

  interface_ = iface;
  interface_ready_ = true;
  return true;
}

bool VulkanEnvironment::context_reset() noexcept {
  if (!render_requested_ || !interface_ready_ || !hw_.context_reset)
    return false;

  reset_in_progress_ = true;
  hw_.context_reset();
  reset_in_progress_ = false;
  context_live_ = true;
  return true;
}

void VulkanEnvironment::context_destroy() noexcept {
  if ((context_live_ || reset_in_progress_) && hw_.context_destroy)
    hw_.context_destroy();

  reset_in_progress_ = false;
  context_live_ = false;
  interface_ready_ = false;
  std::memset(&interface_, 0, sizeof(interface_));
}

void VulkanEnvironment::clear() noexcept {
  context_destroy();
  std::memset(&hw_, 0, sizeof(hw_));
  std::memset(&negotiation_, 0, sizeof(negotiation_));
  render_requested_ = false;
  negotiation_ready_ = false;
}

} // namespace native_emus::ps1
