#pragma once

#include "display_surface.hpp"
#include "vulkan_provider.hpp"

#include <cstdint>
#include <mutex>
#include <vector>

namespace native_emus::ps1 {

struct PresentRect {
  std::int32_t x0{};
  std::int32_t y0{};
  std::int32_t x1{};
  std::int32_t y1{};
};

[[nodiscard]] PresentRect fit_present_rect(
    std::uint32_t source_width,
    std::uint32_t source_height,
    VkExtent2D destination) noexcept;

// Minimal standalone Vulkan presenter for Beetle PSX HW.
//
// The core renders its final image. This class owns only the PS5 display
// swapchain and transfers/blits that final image into the acquired display
// image. No frontend shaders, filters, menus or RetroArch renderer are used.
class VulkanPresenter final {
public:
  explicit VulkanPresenter(
      PFN_vkGetInstanceProcAddr get_instance_proc_addr) noexcept;
  ~VulkanPresenter();

  VulkanPresenter(const VulkanPresenter&) = delete;
  VulkanPresenter& operator=(const VulkanPresenter&) = delete;

  [[nodiscard]] VulkanPresentationHooks hooks();
  [[nodiscard]] bool present(
      std::uint32_t source_width,
      std::uint32_t source_height) noexcept;
  [[nodiscard]] bool ready() const noexcept { return initialized_; }

private:
  struct Frame {
    VkSemaphore acquired{VK_NULL_HANDLE};
    VkSemaphore ready{VK_NULL_HANDLE};
    VkFence fence{VK_NULL_HANDLE};
    VkCommandBuffer command{VK_NULL_HANDLE};
  };

  struct Api {
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR surface_support{};
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR surface_caps{};
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR surface_formats{};

    PFN_vkCreateSwapchainKHR create_swapchain{};
    PFN_vkDestroySwapchainKHR destroy_swapchain{};
    PFN_vkGetSwapchainImagesKHR get_swapchain_images{};
    PFN_vkAcquireNextImageKHR acquire_next_image{};
    PFN_vkQueuePresentKHR queue_present{};

    PFN_vkCreateSemaphore create_semaphore{};
    PFN_vkDestroySemaphore destroy_semaphore{};
    PFN_vkCreateFence create_fence{};
    PFN_vkDestroyFence destroy_fence{};
    PFN_vkWaitForFences wait_fences{};
    PFN_vkResetFences reset_fences{};

    PFN_vkCreateCommandPool create_command_pool{};
    PFN_vkDestroyCommandPool destroy_command_pool{};
    PFN_vkAllocateCommandBuffers allocate_command_buffers{};
    PFN_vkResetCommandBuffer reset_command_buffer{};
    PFN_vkBeginCommandBuffer begin_command_buffer{};
    PFN_vkEndCommandBuffer end_command_buffer{};
    PFN_vkCmdPipelineBarrier pipeline_barrier{};
    PFN_vkCmdClearColorImage clear_color_image{};
    PFN_vkCmdBlitImage blit_image{};

    PFN_vkQueueSubmit queue_submit{};
    PFN_vkDeviceWaitIdle device_wait_idle{};
  };

  bool initialize(
      VkInstance instance,
      VkSurfaceKHR surface,
      const retro_vulkan_context& context,
      PFN_vkGetInstanceProcAddr get_instance_proc_addr,
      PFN_vkGetDeviceProcAddr get_device_proc_addr) noexcept;
  void shutdown() noexcept;

  void set_image(
      const retro_vulkan_image* image,
      std::uint32_t num_semaphores,
      const VkSemaphore* semaphores,
      std::uint32_t src_queue_family);
  std::uint32_t get_sync_index() noexcept;
  std::uint32_t get_sync_index_mask() const noexcept;
  void set_command_buffers(
      std::uint32_t count,
      const VkCommandBuffer* commands);
  void wait_sync_index() noexcept;
  void set_signal_semaphore(VkSemaphore semaphore) noexcept;

  bool load_api() noexcept;
  bool create_swapchain() noexcept;
  bool create_frame_resources() noexcept;
  bool acquire_frame() noexcept;
  bool record_transfer(
      Frame& frame,
      const retro_vulkan_image& source,
      std::uint32_t source_queue_family,
      std::uint32_t source_width,
      std::uint32_t source_height) noexcept;

  PFN_vkGetInstanceProcAddr bootstrap_get_instance_proc_addr_{};
  PFN_vkGetInstanceProcAddr get_instance_proc_addr_{};
  PFN_vkGetDeviceProcAddr get_device_proc_addr_{};
  Api api_{};

  VkInstance instance_{VK_NULL_HANDLE};
  VkPhysicalDevice gpu_{VK_NULL_HANDLE};
  VkDevice device_{VK_NULL_HANDLE};
  VkSurfaceKHR surface_{VK_NULL_HANDLE};
  VkQueue queue_{VK_NULL_HANDLE};
  VkQueue presentation_queue_{VK_NULL_HANDLE};
  std::uint32_t queue_family_{};
  std::uint32_t presentation_queue_family_{};

  VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
  VkFormat swapchain_format_{VK_FORMAT_UNDEFINED};
  VkColorSpaceKHR color_space_{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  VkExtent2D extent_{};
  VkCommandPool command_pool_{VK_NULL_HANDLE};

  std::vector<VkImage> swapchain_images_;
  std::vector<Frame> frames_;
  std::vector<int> image_last_frame_;

  mutable std::mutex queue_mutex_;
  std::mutex state_mutex_;

  retro_vulkan_image source_image_{};
  bool source_valid_{};
  std::uint32_t source_queue_family_{VK_QUEUE_FAMILY_IGNORED};
  std::vector<VkSemaphore> source_wait_semaphores_;
  std::vector<VkCommandBuffer> source_command_buffers_;
  VkSemaphore source_signal_semaphore_{VK_NULL_HANDLE};

  std::uint32_t frame_cursor_{};
  std::uint32_t current_slot_{};
  std::uint32_t current_image_{};
  bool acquired_{};
  bool initialized_{};
};

} // namespace native_emus::ps1
