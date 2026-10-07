#pragma once

#include <libretro.h>
#include <libretro_vulkan.h>

namespace native_emus::ps1 {

// Owns the libretro Vulkan hardware-context lifecycle for the standalone PS1
// frontend. It intentionally does not create a VkInstance/device/swapchain;
// the PS5 Vulkan provider publishes those objects through publish_interface().
class VulkanEnvironment final {
public:
  bool environment(unsigned cmd, void* data) noexcept;

  [[nodiscard]] bool render_requested() const noexcept { return render_requested_; }
  [[nodiscard]] bool negotiation_ready() const noexcept { return negotiation_ready_; }
  [[nodiscard]] bool interface_ready() const noexcept { return interface_ready_; }
  [[nodiscard]] bool context_live() const noexcept { return context_live_; }

  [[nodiscard]] const retro_hw_render_callback& hw_callback() const noexcept {
    return hw_;
  }

  [[nodiscard]] const retro_hw_render_context_negotiation_interface_vulkan*
  negotiation() const noexcept {
    return negotiation_ready_ ? &negotiation_ : nullptr;
  }

  bool publish_interface(const retro_hw_render_interface_vulkan& iface) noexcept;
  bool context_reset() noexcept;
  void context_destroy() noexcept;
  void clear() noexcept;

private:
  retro_hw_render_callback hw_{};
  retro_hw_render_context_negotiation_interface_vulkan negotiation_{};
  retro_hw_render_interface_vulkan interface_{};

  bool render_requested_{};
  bool negotiation_ready_{};
  bool interface_ready_{};
  bool reset_in_progress_{};
  bool context_live_{};
};

} // namespace native_emus::ps1
