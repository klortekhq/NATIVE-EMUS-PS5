#include "vulkan_presenter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

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

template <typename T>
T load_device(
    PFN_vkGetDeviceProcAddr get_proc,
    VkDevice device,
    const char* name) noexcept {
  return get_proc
      ? reinterpret_cast<T>(get_proc(device, name))
      : nullptr;
}

bool good_acquire(VkResult result) noexcept {
  return result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR;
}

} // namespace

PresentRect fit_present_rect(
    std::uint32_t source_width,
    std::uint32_t source_height,
    VkExtent2D destination) noexcept {
  if (!source_width || !source_height ||
      !destination.width || !destination.height)
    return {};

  const double sx =
      static_cast<double>(destination.width) / source_width;
  const double sy =
      static_cast<double>(destination.height) / source_height;
  const double scale = std::min(sx, sy);

  const auto width = std::max<std::uint32_t>(
      1u, static_cast<std::uint32_t>(
              std::llround(static_cast<double>(source_width) * scale)));
  const auto height = std::max<std::uint32_t>(
      1u, static_cast<std::uint32_t>(
              std::llround(static_cast<double>(source_height) * scale)));

  const auto x = static_cast<std::int32_t>(
      (destination.width - std::min(width, destination.width)) / 2u);
  const auto y = static_cast<std::int32_t>(
      (destination.height - std::min(height, destination.height)) / 2u);

  return {
      x,
      y,
      x + static_cast<std::int32_t>(std::min(width, destination.width)),
      y + static_cast<std::int32_t>(std::min(height, destination.height)),
  };
}

VulkanPresenter::VulkanPresenter(
    PFN_vkGetInstanceProcAddr get_instance_proc_addr) noexcept
    : bootstrap_get_instance_proc_addr_(get_instance_proc_addr) {}

VulkanPresenter::~VulkanPresenter() {
  shutdown();
}

VulkanPresentationHooks VulkanPresenter::hooks() {
  VulkanPresentationHooks out{};

  out.create_surface = [this](
      VkInstance instance, VkPhysicalDevice gpu) {
    return create_ps5_display_surface(
        instance, gpu, bootstrap_get_instance_proc_addr_);
  };

  out.initialize = [this](
      VkInstance instance,
      VkSurfaceKHR surface,
      const retro_vulkan_context& context,
      PFN_vkGetInstanceProcAddr gip,
      PFN_vkGetDeviceProcAddr gdp) {
    return initialize(instance, surface, context, gip, gdp);
  };

  out.shutdown = [this] { shutdown(); };

  out.set_image = [this](
      const retro_vulkan_image* image,
      std::uint32_t count,
      const VkSemaphore* semaphores,
      std::uint32_t source_family) {
    set_image(image, count, semaphores, source_family);
  };

  out.get_sync_index = [this] { return get_sync_index(); };
  out.get_sync_index_mask = [this] { return get_sync_index_mask(); };
  out.set_command_buffers = [this](
      std::uint32_t count, const VkCommandBuffer* commands) {
    set_command_buffers(count, commands);
  };
  out.wait_sync_index = [this] { wait_sync_index(); };
  out.set_signal_semaphore = [this](VkSemaphore semaphore) {
    set_signal_semaphore(semaphore);
  };
  out.lock_queue = [this] { queue_mutex_.lock(); };
  out.unlock_queue = [this] { queue_mutex_.unlock(); };

  return out;
}

bool VulkanPresenter::initialize(
    VkInstance instance,
    VkSurfaceKHR surface,
    const retro_vulkan_context& context,
    PFN_vkGetInstanceProcAddr get_instance_proc_addr,
    PFN_vkGetDeviceProcAddr get_device_proc_addr) noexcept {
  if (initialized_ || instance == VK_NULL_HANDLE ||
      surface == VK_NULL_HANDLE || context.gpu == VK_NULL_HANDLE ||
      context.device == VK_NULL_HANDLE || context.queue == VK_NULL_HANDLE ||
      !get_instance_proc_addr || !get_device_proc_addr)
    return false;

  instance_ = instance;
  gpu_ = context.gpu;
  device_ = context.device;
  surface_ = surface;
  queue_ = context.queue;
  queue_family_ = context.queue_family_index;
  presentation_queue_ =
      context.presentation_queue != VK_NULL_HANDLE
          ? context.presentation_queue
          : context.queue;
  presentation_queue_family_ =
      context.presentation_queue != VK_NULL_HANDLE
          ? context.presentation_queue_family_index
          : context.queue_family_index;
  get_instance_proc_addr_ = get_instance_proc_addr;
  get_device_proc_addr_ = get_device_proc_addr;

  if (!load_api() || !create_swapchain() || !create_frame_resources()) {
    shutdown();
    return false;
  }

  initialized_ = true;
  return true;
}

void VulkanPresenter::shutdown() noexcept {
  std::scoped_lock queue_lock(queue_mutex_);

  if (device_ != VK_NULL_HANDLE && api_.device_wait_idle)
    (void)api_.device_wait_idle(device_);

  if (device_ != VK_NULL_HANDLE) {
    for (auto& frame : frames_) {
      if (frame.fence != VK_NULL_HANDLE && api_.destroy_fence)
        api_.destroy_fence(device_, frame.fence, nullptr);
      if (frame.acquired != VK_NULL_HANDLE && api_.destroy_semaphore)
        api_.destroy_semaphore(device_, frame.acquired, nullptr);
      if (frame.ready != VK_NULL_HANDLE && api_.destroy_semaphore)
        api_.destroy_semaphore(device_, frame.ready, nullptr);
    }

    if (command_pool_ != VK_NULL_HANDLE && api_.destroy_command_pool)
      api_.destroy_command_pool(device_, command_pool_, nullptr);

    if (swapchain_ != VK_NULL_HANDLE && api_.destroy_swapchain)
      api_.destroy_swapchain(device_, swapchain_, nullptr);
  }

  {
    std::scoped_lock state_lock(state_mutex_);
    source_image_ = {};
    source_valid_ = false;
    source_queue_family_ = VK_QUEUE_FAMILY_IGNORED;
    source_wait_semaphores_.clear();
    source_command_buffers_.clear();
    source_signal_semaphore_ = VK_NULL_HANDLE;
  }

  frames_.clear();
  swapchain_images_.clear();
  image_last_frame_.clear();
  api_ = {};
  instance_ = VK_NULL_HANDLE;
  gpu_ = VK_NULL_HANDLE;
  device_ = VK_NULL_HANDLE;
  surface_ = VK_NULL_HANDLE;
  queue_ = VK_NULL_HANDLE;
  presentation_queue_ = VK_NULL_HANDLE;
  swapchain_ = VK_NULL_HANDLE;
  command_pool_ = VK_NULL_HANDLE;
  swapchain_format_ = VK_FORMAT_UNDEFINED;
  extent_ = {};
  get_instance_proc_addr_ = nullptr;
  get_device_proc_addr_ = nullptr;
  frame_cursor_ = 0;
  current_slot_ = 0;
  current_image_ = 0;
  acquired_ = false;
  initialized_ = false;
}

bool VulkanPresenter::load_api() noexcept {
#define LOAD_I(member, type, name)   api_.member = load_instance<type>(get_instance_proc_addr_, instance_, name)
#define LOAD_D(member, type, name)   api_.member = load_device<type>(get_device_proc_addr_, device_, name)

  LOAD_I(surface_support, PFN_vkGetPhysicalDeviceSurfaceSupportKHR,
         "vkGetPhysicalDeviceSurfaceSupportKHR");
  LOAD_I(surface_caps, PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR,
         "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
  LOAD_I(surface_formats, PFN_vkGetPhysicalDeviceSurfaceFormatsKHR,
         "vkGetPhysicalDeviceSurfaceFormatsKHR");

  LOAD_D(create_swapchain, PFN_vkCreateSwapchainKHR, "vkCreateSwapchainKHR");
  LOAD_D(destroy_swapchain, PFN_vkDestroySwapchainKHR, "vkDestroySwapchainKHR");
  LOAD_D(get_swapchain_images, PFN_vkGetSwapchainImagesKHR,
         "vkGetSwapchainImagesKHR");
  LOAD_D(acquire_next_image, PFN_vkAcquireNextImageKHR,
         "vkAcquireNextImageKHR");
  LOAD_D(queue_present, PFN_vkQueuePresentKHR, "vkQueuePresentKHR");
  LOAD_D(create_semaphore, PFN_vkCreateSemaphore, "vkCreateSemaphore");
  LOAD_D(destroy_semaphore, PFN_vkDestroySemaphore, "vkDestroySemaphore");
  LOAD_D(create_fence, PFN_vkCreateFence, "vkCreateFence");
  LOAD_D(destroy_fence, PFN_vkDestroyFence, "vkDestroyFence");
  LOAD_D(wait_fences, PFN_vkWaitForFences, "vkWaitForFences");
  LOAD_D(reset_fences, PFN_vkResetFences, "vkResetFences");
  LOAD_D(create_command_pool, PFN_vkCreateCommandPool, "vkCreateCommandPool");
  LOAD_D(destroy_command_pool, PFN_vkDestroyCommandPool,
         "vkDestroyCommandPool");
  LOAD_D(allocate_command_buffers, PFN_vkAllocateCommandBuffers,
         "vkAllocateCommandBuffers");
  LOAD_D(reset_command_buffer, PFN_vkResetCommandBuffer,
         "vkResetCommandBuffer");
  LOAD_D(begin_command_buffer, PFN_vkBeginCommandBuffer,
         "vkBeginCommandBuffer");
  LOAD_D(end_command_buffer, PFN_vkEndCommandBuffer, "vkEndCommandBuffer");
  LOAD_D(pipeline_barrier, PFN_vkCmdPipelineBarrier, "vkCmdPipelineBarrier");
  LOAD_D(clear_color_image, PFN_vkCmdClearColorImage, "vkCmdClearColorImage");
  LOAD_D(blit_image, PFN_vkCmdBlitImage, "vkCmdBlitImage");
  LOAD_D(queue_submit, PFN_vkQueueSubmit, "vkQueueSubmit");
  LOAD_D(device_wait_idle, PFN_vkDeviceWaitIdle, "vkDeviceWaitIdle");

#undef LOAD_I
#undef LOAD_D

  return api_.surface_support && api_.surface_caps && api_.surface_formats &&
         api_.create_swapchain && api_.destroy_swapchain &&
         api_.get_swapchain_images && api_.acquire_next_image &&
         api_.queue_present && api_.create_semaphore &&
         api_.destroy_semaphore && api_.create_fence &&
         api_.destroy_fence && api_.wait_fences && api_.reset_fences &&
         api_.create_command_pool && api_.destroy_command_pool &&
         api_.allocate_command_buffers && api_.reset_command_buffer &&
         api_.begin_command_buffer && api_.end_command_buffer &&
         api_.pipeline_barrier && api_.clear_color_image &&
         api_.blit_image && api_.queue_submit && api_.device_wait_idle;
}

bool VulkanPresenter::create_swapchain() noexcept {
  VkBool32 presentation_supported = VK_FALSE;
  if (api_.surface_support(
          gpu_, presentation_queue_family_, surface_,
          &presentation_supported) != VK_SUCCESS ||
      presentation_supported != VK_TRUE)
    return false;

  VkSurfaceCapabilitiesKHR caps{};
  if (api_.surface_caps(gpu_, surface_, &caps) != VK_SUCCESS)
    return false;
  if ((caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0)
    return false;

  std::uint32_t format_count = 0;
  if (api_.surface_formats(gpu_, surface_, &format_count, nullptr) !=
          VK_SUCCESS ||
      format_count == 0)
    return false;

  std::vector<VkSurfaceFormatKHR> formats(format_count);
  if (api_.surface_formats(
          gpu_, surface_, &format_count, formats.data()) != VK_SUCCESS ||
      format_count == 0)
    return false;

  VkSurfaceFormatKHR chosen = formats.front();
  for (const auto& format : formats) {
    if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      chosen = format;
      break;
    }
  }

  swapchain_format_ = chosen.format;
  color_space_ = chosen.colorSpace;

  extent_ = caps.currentExtent;
  if (extent_.width == std::numeric_limits<std::uint32_t>::max() ||
      extent_.height == std::numeric_limits<std::uint32_t>::max()) {
    extent_.width = std::clamp<std::uint32_t>(
        3840u, caps.minImageExtent.width, caps.maxImageExtent.width);
    extent_.height = std::clamp<std::uint32_t>(
        2160u, caps.minImageExtent.height, caps.maxImageExtent.height);
  }

  std::uint32_t image_count = std::max(3u, caps.minImageCount);
  if (caps.maxImageCount != 0)
    image_count = std::min(image_count, caps.maxImageCount);

  const std::array<std::uint32_t, 2> queue_families{
      queue_family_, presentation_queue_family_};

  VkSwapchainCreateInfoKHR info{};
  info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  info.surface = surface_;
  info.minImageCount = image_count;
  info.imageFormat = swapchain_format_;
  info.imageColorSpace = color_space_;
  info.imageExtent = extent_;
  info.imageArrayLayers = 1;
  info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  if (caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

  if (queue_family_ != presentation_queue_family_) {
    info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    info.queueFamilyIndexCount = 2;
    info.pQueueFamilyIndices = queue_families.data();
  } else {
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  info.preTransform =
      (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
          ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
          : caps.currentTransform;
  info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  info.clipped = VK_TRUE;

  if (api_.create_swapchain(device_, &info, nullptr, &swapchain_) !=
          VK_SUCCESS ||
      swapchain_ == VK_NULL_HANDLE)
    return false;

  std::uint32_t count = 0;
  if (api_.get_swapchain_images(
          device_, swapchain_, &count, nullptr) != VK_SUCCESS ||
      count == 0 || count > 31)
    return false;

  swapchain_images_.resize(count);
  if (api_.get_swapchain_images(
          device_, swapchain_, &count, swapchain_images_.data()) !=
          VK_SUCCESS ||
      count == 0)
    return false;

  swapchain_images_.resize(count);
  image_last_frame_.assign(count, -1);
  return true;
}

bool VulkanPresenter::create_frame_resources() noexcept {
  if (swapchain_images_.empty())
    return false;

  VkCommandPoolCreateInfo pool{};
  pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool.queueFamilyIndex = queue_family_;
  if (api_.create_command_pool(
          device_, &pool, nullptr, &command_pool_) != VK_SUCCESS)
    return false;

  frames_.resize(swapchain_images_.size());
  std::vector<VkCommandBuffer> commands(frames_.size());

  VkCommandBufferAllocateInfo alloc{};
  alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc.commandPool = command_pool_;
  alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc.commandBufferCount =
      static_cast<std::uint32_t>(commands.size());
  if (api_.allocate_command_buffers(device_, &alloc, commands.data()) !=
      VK_SUCCESS)
    return false;

  const VkSemaphoreCreateInfo semaphore_info{
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (std::size_t i = 0; i < frames_.size(); ++i) {
    frames_[i].command = commands[i];
    if (api_.create_semaphore(
            device_, &semaphore_info, nullptr,
            &frames_[i].acquired) != VK_SUCCESS ||
        api_.create_semaphore(
            device_, &semaphore_info, nullptr,
            &frames_[i].ready) != VK_SUCCESS ||
        api_.create_fence(
            device_, &fence_info, nullptr,
            &frames_[i].fence) != VK_SUCCESS)
      return false;
  }

  return true;
}

bool VulkanPresenter::acquire_frame() noexcept {
  if (!initialized_ || acquired_ || frames_.empty())
    return acquired_;

  current_slot_ =
      frame_cursor_ % static_cast<std::uint32_t>(frames_.size());
  auto& frame = frames_[current_slot_];

  if (api_.wait_fences(
          device_, 1, &frame.fence, VK_TRUE,
          std::numeric_limits<std::uint64_t>::max()) != VK_SUCCESS)
    return false;

  std::uint32_t image_index = 0;
  const VkResult result = api_.acquire_next_image(
      device_, swapchain_,
      std::numeric_limits<std::uint64_t>::max(),
      frame.acquired, VK_NULL_HANDLE, &image_index);
  if (!good_acquire(result) ||
      image_index >= swapchain_images_.size())
    return false;

  current_image_ = image_index;
  acquired_ = true;
  return true;
}

std::uint32_t VulkanPresenter::get_sync_index() noexcept {
  if (!acquired_ && !acquire_frame())
    return 0;
  return current_image_;
}

std::uint32_t VulkanPresenter::get_sync_index_mask() const noexcept {
  if (swapchain_images_.empty() || swapchain_images_.size() > 31)
    return 1u;
  return (std::uint32_t{1} << swapchain_images_.size()) - 1u;
}

void VulkanPresenter::wait_sync_index() noexcept {
  if (!acquired_ || current_image_ >= image_last_frame_.size())
    return;

  const int previous = image_last_frame_[current_image_];
  if (previous < 0 ||
      static_cast<std::size_t>(previous) >= frames_.size())
    return;

  const auto& frame = frames_[static_cast<std::size_t>(previous)];
  (void)api_.wait_fences(
      device_, 1, &frame.fence, VK_TRUE,
      std::numeric_limits<std::uint64_t>::max());
}

void VulkanPresenter::set_image(
    const retro_vulkan_image* image,
    std::uint32_t num_semaphores,
    const VkSemaphore* semaphores,
    std::uint32_t src_queue_family) {
  std::scoped_lock lock(state_mutex_);
  source_valid_ = image != nullptr;
  source_image_ = image ? *image : retro_vulkan_image{};
  source_queue_family_ = src_queue_family;
  source_wait_semaphores_.clear();
  if (semaphores && num_semaphores)
    source_wait_semaphores_.assign(
        semaphores, semaphores + num_semaphores);
}

void VulkanPresenter::set_command_buffers(
    std::uint32_t count,
    const VkCommandBuffer* commands) {
  std::scoped_lock lock(state_mutex_);
  source_command_buffers_.clear();
  if (commands && count)
    source_command_buffers_.assign(commands, commands + count);
}

void VulkanPresenter::set_signal_semaphore(
    VkSemaphore semaphore) noexcept {
  std::scoped_lock lock(state_mutex_);
  source_signal_semaphore_ = semaphore;
}

bool VulkanPresenter::record_transfer(
    Frame& frame,
    const retro_vulkan_image& source,
    std::uint32_t source_queue_family,
    std::uint32_t source_width,
    std::uint32_t source_height) noexcept {
  if (source.create_info.image == VK_NULL_HANDLE ||
      current_image_ >= swapchain_images_.size() ||
      !source_width || !source_height)
    return false;

  if (api_.reset_command_buffer(frame.command, 0) != VK_SUCCESS)
    return false;

  VkCommandBufferBeginInfo begin{};
  begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (api_.begin_command_buffer(frame.command, &begin) != VK_SUCCESS)
    return false;

  const bool transfer_ownership =
      source_queue_family != VK_QUEUE_FAMILY_IGNORED &&
      source_queue_family != queue_family_;

  VkImageSubresourceRange src_range = source.create_info.subresourceRange;
  if (!src_range.aspectMask)
    src_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  if (!src_range.levelCount)
    src_range.levelCount = 1;
  if (!src_range.layerCount)
    src_range.layerCount = 1;

  VkImageMemoryBarrier before[2]{};

  before[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  before[0].srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  before[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  before[0].oldLayout = source.image_layout;
  before[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  before[0].srcQueueFamilyIndex =
      transfer_ownership ? source_queue_family : VK_QUEUE_FAMILY_IGNORED;
  before[0].dstQueueFamilyIndex =
      transfer_ownership ? queue_family_ : VK_QUEUE_FAMILY_IGNORED;
  before[0].image = source.create_info.image;
  before[0].subresourceRange = src_range;

  before[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  before[1].srcAccessMask = 0;
  before[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  before[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  before[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  before[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  before[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  before[1].image = swapchain_images_[current_image_];
  before[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  before[1].subresourceRange.baseMipLevel = 0;
  before[1].subresourceRange.levelCount = 1;
  before[1].subresourceRange.baseArrayLayer = 0;
  before[1].subresourceRange.layerCount = 1;

  api_.pipeline_barrier(
      frame.command,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0, 0, nullptr, 0, nullptr, 2, before);

  const VkClearColorValue black{{0.0f, 0.0f, 0.0f, 1.0f}};
  const VkImageSubresourceRange dst_range{
      VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  api_.clear_color_image(
      frame.command,
      swapchain_images_[current_image_],
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      &black, 1, &dst_range);

  VkImageBlit blit{};
  blit.srcSubresource.aspectMask = src_range.aspectMask;
  blit.srcSubresource.mipLevel = src_range.baseMipLevel;
  blit.srcSubresource.baseArrayLayer = src_range.baseArrayLayer;
  blit.srcSubresource.layerCount = 1;
  blit.srcOffsets[1] = {
      static_cast<std::int32_t>(source_width),
      static_cast<std::int32_t>(source_height), 1};

  const PresentRect rect =
      fit_present_rect(source_width, source_height, extent_);
  blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit.dstSubresource.mipLevel = 0;
  blit.dstSubresource.baseArrayLayer = 0;
  blit.dstSubresource.layerCount = 1;
  blit.dstOffsets[0] = {rect.x0, rect.y0, 0};
  blit.dstOffsets[1] = {rect.x1, rect.y1, 1};

  api_.blit_image(
      frame.command,
      source.create_info.image,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      swapchain_images_[current_image_],
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1, &blit, VK_FILTER_NEAREST);

  VkImageMemoryBarrier after[2]{};

  after[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  after[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  after[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT |
                           VK_ACCESS_MEMORY_WRITE_BIT;
  after[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  after[0].newLayout = source.image_layout;
  after[0].srcQueueFamilyIndex =
      transfer_ownership ? queue_family_ : VK_QUEUE_FAMILY_IGNORED;
  after[0].dstQueueFamilyIndex =
      transfer_ownership ? source_queue_family : VK_QUEUE_FAMILY_IGNORED;
  after[0].image = source.create_info.image;
  after[0].subresourceRange = src_range;

  after[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  after[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  after[1].dstAccessMask = 0;
  after[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  after[1].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  after[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  after[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  after[1].image = swapchain_images_[current_image_];
  after[1].subresourceRange = dst_range;

  api_.pipeline_barrier(
      frame.command,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      0, 0, nullptr, 0, nullptr, 2, after);

  return api_.end_command_buffer(frame.command) == VK_SUCCESS;
}

bool VulkanPresenter::present(
    std::uint32_t source_width,
    std::uint32_t source_height) noexcept {
  if (!initialized_)
    return false;
  if (!acquired_ && !acquire_frame())
    return false;

  wait_sync_index();

  retro_vulkan_image source{};
  std::uint32_t source_family = VK_QUEUE_FAMILY_IGNORED;
  std::vector<VkSemaphore> waits;
  std::vector<VkCommandBuffer> core_commands;
  VkSemaphore extra_signal = VK_NULL_HANDLE;

  {
    std::scoped_lock state_lock(state_mutex_);
    if (!source_valid_)
      return false;
    source = source_image_;
    source_family = source_queue_family_;
    waits = source_wait_semaphores_;
    core_commands = source_command_buffers_;
    extra_signal = source_signal_semaphore_;
  }

  auto& frame = frames_[current_slot_];
  if (!record_transfer(
          frame, source, source_family,
          source_width, source_height))
    return false;

  std::vector<VkSemaphore> submit_waits;
  submit_waits.reserve(waits.size() + 1);
  submit_waits.push_back(frame.acquired);
  submit_waits.insert(
      submit_waits.end(), waits.begin(), waits.end());

  std::vector<VkPipelineStageFlags> wait_stages(
      submit_waits.size(), VK_PIPELINE_STAGE_TRANSFER_BIT);

  std::vector<VkCommandBuffer> commands = std::move(core_commands);
  commands.push_back(frame.command);

  std::vector<VkSemaphore> signals;
  signals.push_back(frame.ready);
  if (extra_signal != VK_NULL_HANDLE)
    signals.push_back(extra_signal);

  VkSubmitInfo submit{};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.waitSemaphoreCount =
      static_cast<std::uint32_t>(submit_waits.size());
  submit.pWaitSemaphores = submit_waits.data();
  submit.pWaitDstStageMask = wait_stages.data();
  submit.commandBufferCount =
      static_cast<std::uint32_t>(commands.size());
  submit.pCommandBuffers = commands.data();
  submit.signalSemaphoreCount =
      static_cast<std::uint32_t>(signals.size());
  submit.pSignalSemaphores = signals.data();

  if (api_.reset_fences(device_, 1, &frame.fence) != VK_SUCCESS)
    return false;

  VkResult submit_result = VK_ERROR_DEVICE_LOST;
  VkResult present_result = VK_ERROR_DEVICE_LOST;
  {
    std::scoped_lock queue_lock(queue_mutex_);
    submit_result =
        api_.queue_submit(queue_, 1, &submit, frame.fence);
    if (submit_result == VK_SUCCESS) {
      VkPresentInfoKHR present_info{};
      present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      present_info.waitSemaphoreCount = 1;
      present_info.pWaitSemaphores = &frame.ready;
      present_info.swapchainCount = 1;
      present_info.pSwapchains = &swapchain_;
      present_info.pImageIndices = &current_image_;
      present_result =
          api_.queue_present(presentation_queue_, &present_info);
    }
  }

  if (submit_result != VK_SUCCESS)
    return false;

  image_last_frame_[current_image_] =
      static_cast<int>(current_slot_);

  acquired_ = false;
  ++frame_cursor_;

  {
    std::scoped_lock state_lock(state_mutex_);
    source_wait_semaphores_.clear();
    source_command_buffers_.clear();
    source_signal_semaphore_ = VK_NULL_HANDLE;
  }

  return present_result == VK_SUCCESS ||
         present_result == VK_SUBOPTIMAL_KHR;
}

} // namespace native_emus::ps1
