#pragma once

#include "vulkan_environment.hpp"

#include <functional>
#include <mutex>

namespace native_emus::ps1 {

struct VulkanPresentationHooks {
  // Called after the Vulkan instance and physical device exist.
  std::function<VkSurfaceKHR(VkInstance, VkPhysicalDevice)> create_surface;

  std::function<void(const retro_vulkan_image*, uint32_t,
                     const VkSemaphore*, uint32_t)> set_image;
  std::function<uint32_t()> get_sync_index;
  std::function<uint32_t()> get_sync_index_mask;
  std::function<void(uint32_t, const VkCommandBuffer*)> set_command_buffers;
  std::function<void()> wait_sync_index;
  std::function<void(VkSemaphore)> set_signal_semaphore;
};

class VulkanProvider final {
public:
  VulkanProvider(VulkanEnvironment& environment,
                 VulkanPresentationHooks presentation) noexcept;
  ~VulkanProvider();

  VulkanProvider(const VulkanProvider&) = delete;
  VulkanProvider& operator=(const VulkanProvider&) = delete;

  bool initialize(PFN_vkGetInstanceProcAddr get_instance_proc_addr) noexcept;
  void shutdown() noexcept;

  [[nodiscard]] bool initialized() const noexcept { return initialized_; }
  [[nodiscard]] VkInstance instance() const noexcept { return instance_; }
  [[nodiscard]] VkPhysicalDevice gpu() const noexcept { return context_.gpu; }
  [[nodiscard]] VkDevice device() const noexcept { return context_.device; }
  [[nodiscard]] VkSurfaceKHR surface() const noexcept { return surface_; }

private:
  static VkInstance create_instance_wrapper(
      void* opaque, const VkInstanceCreateInfo* create_info);
  static VkDevice create_device_wrapper(
      VkPhysicalDevice gpu, void* opaque,
      const VkDeviceCreateInfo* create_info);

  static void set_image(void* handle, const retro_vulkan_image* image,
                        uint32_t num_semaphores,
                        const VkSemaphore* semaphores,
                        uint32_t src_queue_family);
  static uint32_t get_sync_index(void* handle);
  static uint32_t get_sync_index_mask(void* handle);
  static void set_command_buffers(void* handle, uint32_t num_cmd,
                                  const VkCommandBuffer* cmd);
  static void wait_sync_index(void* handle);
  static void lock_queue(void* handle);
  static void unlock_queue(void* handle);
  static void set_signal_semaphore(void* handle, VkSemaphore semaphore);

  bool create_instance_from_negotiation() noexcept;
  bool choose_gpu() noexcept;
  bool create_device_from_negotiation() noexcept;
  bool publish_interface() noexcept;

  VulkanEnvironment& environment_;
  VulkanPresentationHooks presentation_;

  PFN_vkGetInstanceProcAddr get_instance_proc_addr_{};
  PFN_vkGetDeviceProcAddr get_device_proc_addr_{};

  VkInstance instance_{VK_NULL_HANDLE};
  VkSurfaceKHR surface_{VK_NULL_HANDLE};
  retro_vulkan_context context_{};
  retro_hw_render_interface_vulkan interface_{};

  std::mutex queue_mutex_;
  bool device_created_by_core_{};
  bool initialized_{};
};

} // namespace native_emus::ps1
