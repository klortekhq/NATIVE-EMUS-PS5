#pragma once

#include <cstdint>

#include <ps5rt/result.hpp>

namespace ps5rt {

// Deliberately opaque. Emulator renderers should keep using Vulkan directly;
// ps5rt only supplies the PS5 bootstrap/native-surface plumbing.
struct VulkanBootstrap {
  void* instance{};
  void* physical_device{};
  void* device{};
  void* queue{};
  std::uint32_t queue_family{};
};

struct VulkanConfig {
  bool enable_validation{false};
  bool enable_shader_cache{true};
  bool prefer_threaded_recording{false};
};

Result create_vulkan_bootstrap(
    const VulkanConfig& config,
    VulkanBootstrap& out) noexcept;

void destroy_vulkan_bootstrap(VulkanBootstrap& bootstrap) noexcept;

} // namespace ps5rt
