#include "vulkan_provider.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

namespace native_emus::ps1 {
namespace {

template <typename T>
T load_instance(PFN_vkGetInstanceProcAddr get_proc,
                VkInstance instance,
                const char* name) noexcept {
  return get_proc
      ? reinterpret_cast<T>(get_proc(instance, name))
      : nullptr;
}

bool contains_extension(const char* const* names,
                        uint32_t count,
                        const char* wanted) noexcept {
  for (uint32_t i = 0; i < count; ++i) {
    if (names[i] && std::strcmp(names[i], wanted) == 0)
      return true;
  }
  return false;
}

} // namespace

VulkanProvider::VulkanProvider(
    VulkanEnvironment& environment,
    VulkanPresentationHooks presentation) noexcept
    : environment_(environment), presentation_(std::move(presentation)) {}

VulkanProvider::~VulkanProvider() {
  shutdown();
}

bool VulkanProvider::initialize(
    PFN_vkGetInstanceProcAddr get_instance_proc_addr) noexcept {
  if (initialized_ || !get_instance_proc_addr ||
      !environment_.render_requested() ||
      !environment_.negotiation_ready())
    return false;

  get_instance_proc_addr_ = get_instance_proc_addr;

  if (!create_instance_from_negotiation())
    return false;
  if (!choose_gpu()) {
    shutdown();
    return false;
  }

  if (presentation_.create_surface)
    surface_ = presentation_.create_surface(instance_, context_.gpu);

  if (!create_device_from_negotiation()) {
    shutdown();
    return false;
  }
  if (presentation_.initialize &&
      !presentation_.initialize(
          instance_, surface_, context_,
          get_instance_proc_addr_, get_device_proc_addr_)) {
    shutdown();
    return false;
  }
  if (!publish_interface()) {
    shutdown();
    return false;
  }
  if (!environment_.context_reset()) {
    shutdown();
    return false;
  }

  initialized_ = true;
  return true;
}

void VulkanProvider::shutdown() noexcept {
  if (environment_.context_live())
    environment_.context_destroy();

  if (presentation_.shutdown)
    presentation_.shutdown();

  const auto* negotiation = environment_.negotiation();

  if (context_.device != VK_NULL_HANDLE) {
    if (device_created_by_core_ && negotiation && negotiation->destroy_device)
      negotiation->destroy_device();

    if (auto destroy_device =
            load_instance<PFN_vkDestroyDevice>(
                get_instance_proc_addr_, instance_, "vkDestroyDevice")) {
      destroy_device(context_.device, nullptr);
    }
  }

  if (surface_ != VK_NULL_HANDLE) {
    if (auto destroy_surface =
            load_instance<PFN_vkDestroySurfaceKHR>(
                get_instance_proc_addr_, instance_, "vkDestroySurfaceKHR")) {
      destroy_surface(instance_, surface_, nullptr);
    }
  }

  if (instance_ != VK_NULL_HANDLE) {
    if (auto destroy_instance =
            load_instance<PFN_vkDestroyInstance>(
                get_instance_proc_addr_, instance_, "vkDestroyInstance")) {
      destroy_instance(instance_, nullptr);
    }
  }

  std::memset(&context_, 0, sizeof(context_));
  std::memset(&interface_, 0, sizeof(interface_));
  instance_ = VK_NULL_HANDLE;
  surface_ = VK_NULL_HANDLE;
  get_device_proc_addr_ = nullptr;
  get_instance_proc_addr_ = nullptr;
  device_created_by_core_ = false;
  initialized_ = false;
}

VkInstance VulkanProvider::create_instance_wrapper(
    void* opaque, const VkInstanceCreateInfo* create_info) {
  auto* self = static_cast<VulkanProvider*>(opaque);
  if (!self || !self->get_instance_proc_addr_ || !create_info)
    return VK_NULL_HANDLE;

  auto create_instance = load_instance<PFN_vkCreateInstance>(
      self->get_instance_proc_addr_, VK_NULL_HANDLE, "vkCreateInstance");
  if (!create_instance)
    return VK_NULL_HANDLE;

  std::vector<const char*> extensions;
  extensions.reserve(create_info->enabledExtensionCount + 2);
  for (uint32_t i = 0; i < create_info->enabledExtensionCount; ++i)
    extensions.push_back(create_info->ppEnabledExtensionNames[i]);

  if (!contains_extension(extensions.data(),
                          static_cast<uint32_t>(extensions.size()),
                          VK_KHR_SURFACE_EXTENSION_NAME))
    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  if (!contains_extension(extensions.data(),
                          static_cast<uint32_t>(extensions.size()),
                          VK_KHR_DISPLAY_EXTENSION_NAME))
    extensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);

  VkInstanceCreateInfo info = *create_info;
  info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  info.ppEnabledExtensionNames = extensions.data();

  VkInstance instance = VK_NULL_HANDLE;
  return create_instance(&info, nullptr, &instance) == VK_SUCCESS
      ? instance
      : VK_NULL_HANDLE;
}

VkDevice VulkanProvider::create_device_wrapper(
    VkPhysicalDevice gpu,
    void* opaque,
    const VkDeviceCreateInfo* create_info) {
  auto* self = static_cast<VulkanProvider*>(opaque);
  if (!self || !self->get_instance_proc_addr_ || !create_info)
    return VK_NULL_HANDLE;

  auto create_device = load_instance<PFN_vkCreateDevice>(
      self->get_instance_proc_addr_, self->instance_, "vkCreateDevice");
  if (!create_device)
    return VK_NULL_HANDLE;

  std::vector<const char*> extensions;
  extensions.reserve(create_info->enabledExtensionCount + 1);
  for (uint32_t i = 0; i < create_info->enabledExtensionCount; ++i)
    extensions.push_back(create_info->ppEnabledExtensionNames[i]);

  if (!contains_extension(extensions.data(),
                          static_cast<uint32_t>(extensions.size()),
                          VK_KHR_SWAPCHAIN_EXTENSION_NAME))
    extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  VkDeviceCreateInfo info = *create_info;
  info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  info.ppEnabledExtensionNames = extensions.data();

  VkDevice device = VK_NULL_HANDLE;
  return create_device(gpu, &info, nullptr, &device) == VK_SUCCESS
      ? device
      : VK_NULL_HANDLE;
}

bool VulkanProvider::create_instance_from_negotiation() noexcept {
  const auto* negotiation = environment_.negotiation();
  if (!negotiation)
    return false;

  VkApplicationInfo fallback{
      VK_STRUCTURE_TYPE_APPLICATION_INFO,
      nullptr,
      "NATIVE-EMUS-PS5 PS1",
      1,
      "NATIVE-EMUS-PS5",
      1,
      VK_API_VERSION_1_1,
  };

  const VkApplicationInfo* app =
      negotiation->get_application_info
          ? negotiation->get_application_info()
          : &fallback;
  if (!app)
    app = &fallback;

  if (negotiation->interface_version >= 2 && negotiation->create_instance) {
    instance_ = negotiation->create_instance(
        get_instance_proc_addr_, app,
        &VulkanProvider::create_instance_wrapper, this);
  } else {
    VkInstanceCreateInfo info{
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr,
        0,
        app,
        0,
        nullptr,
        0,
        nullptr,
    };
    instance_ = create_instance_wrapper(this, &info);
  }

  return instance_ != VK_NULL_HANDLE;
}

bool VulkanProvider::choose_gpu() noexcept {
  auto enumerate = load_instance<PFN_vkEnumeratePhysicalDevices>(
      get_instance_proc_addr_, instance_, "vkEnumeratePhysicalDevices");
  if (!enumerate)
    return false;

  uint32_t count = 0;
  if (enumerate(instance_, &count, nullptr) != VK_SUCCESS || count == 0)
    return false;

  std::vector<VkPhysicalDevice> devices(count);
  if (enumerate(instance_, &count, devices.data()) != VK_SUCCESS || count == 0)
    return false;

  context_.gpu = devices.front();
  return context_.gpu != VK_NULL_HANDLE;
}

bool VulkanProvider::create_device_from_negotiation() noexcept {
  const auto* negotiation = environment_.negotiation();
  if (!negotiation)
    return false;

  bool ok = false;
  if (negotiation->interface_version >= 2 && negotiation->create_device2) {
    ok = negotiation->create_device2(
        &context_, instance_, context_.gpu, surface_,
        get_instance_proc_addr_,
        &VulkanProvider::create_device_wrapper, this);
  } else if (negotiation->create_device) {
    const char* required[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkPhysicalDeviceFeatures features{};
    ok = negotiation->create_device(
        &context_, instance_, context_.gpu, surface_,
        get_instance_proc_addr_,
        required, 1, nullptr, 0, &features);
  } else {
    return false;
  }

  if (!ok || context_.device == VK_NULL_HANDLE ||
      context_.queue == VK_NULL_HANDLE)
    return false;

  device_created_by_core_ = true;
  get_device_proc_addr_ = load_instance<PFN_vkGetDeviceProcAddr>(
      get_instance_proc_addr_, instance_, "vkGetDeviceProcAddr");
  return get_device_proc_addr_ != nullptr;
}

bool VulkanProvider::publish_interface() noexcept {
  interface_.interface_type = RETRO_HW_RENDER_INTERFACE_VULKAN;
  interface_.interface_version = RETRO_HW_RENDER_INTERFACE_VULKAN_VERSION;
  interface_.handle = this;
  interface_.instance = instance_;
  interface_.gpu = context_.gpu;
  interface_.device = context_.device;
  interface_.get_device_proc_addr = get_device_proc_addr_;
  interface_.get_instance_proc_addr = get_instance_proc_addr_;
  interface_.queue = context_.queue;
  interface_.queue_index = context_.queue_family_index;
  interface_.set_image = &VulkanProvider::set_image;
  interface_.get_sync_index = &VulkanProvider::get_sync_index;
  interface_.get_sync_index_mask = &VulkanProvider::get_sync_index_mask;
  interface_.set_command_buffers = &VulkanProvider::set_command_buffers;
  interface_.wait_sync_index = &VulkanProvider::wait_sync_index;
  interface_.lock_queue = &VulkanProvider::lock_queue;
  interface_.unlock_queue = &VulkanProvider::unlock_queue;
  interface_.set_signal_semaphore = &VulkanProvider::set_signal_semaphore;
  return environment_.publish_interface(interface_);
}

void VulkanProvider::set_image(
    void* handle, const retro_vulkan_image* image,
    uint32_t num_semaphores, const VkSemaphore* semaphores,
    uint32_t src_queue_family) {
  auto* self = static_cast<VulkanProvider*>(handle);
  if (self && self->presentation_.set_image)
    self->presentation_.set_image(
        image, num_semaphores, semaphores, src_queue_family);
}

uint32_t VulkanProvider::get_sync_index(void* handle) {
  auto* self = static_cast<VulkanProvider*>(handle);
  return self && self->presentation_.get_sync_index
      ? self->presentation_.get_sync_index() : 0;
}

uint32_t VulkanProvider::get_sync_index_mask(void* handle) {
  auto* self = static_cast<VulkanProvider*>(handle);
  return self && self->presentation_.get_sync_index_mask
      ? self->presentation_.get_sync_index_mask() : 1u;
}

void VulkanProvider::set_command_buffers(
    void* handle, uint32_t num_cmd, const VkCommandBuffer* cmd) {
  auto* self = static_cast<VulkanProvider*>(handle);
  if (self && self->presentation_.set_command_buffers)
    self->presentation_.set_command_buffers(num_cmd, cmd);
}

void VulkanProvider::wait_sync_index(void* handle) {
  auto* self = static_cast<VulkanProvider*>(handle);
  if (self && self->presentation_.wait_sync_index)
    self->presentation_.wait_sync_index();
}

void VulkanProvider::lock_queue(void* handle) {
  if (auto* self = static_cast<VulkanProvider*>(handle)) {
    if (self->presentation_.lock_queue)
      self->presentation_.lock_queue();
    else
      self->queue_mutex_.lock();
  }
}

void VulkanProvider::unlock_queue(void* handle) {
  if (auto* self = static_cast<VulkanProvider*>(handle)) {
    if (self->presentation_.unlock_queue)
      self->presentation_.unlock_queue();
    else
      self->queue_mutex_.unlock();
  }
}

void VulkanProvider::set_signal_semaphore(
    void* handle, VkSemaphore semaphore) {
  auto* self = static_cast<VulkanProvider*>(handle);
  if (self && self->presentation_.set_signal_semaphore)
    self->presentation_.set_signal_semaphore(semaphore);
}

} // namespace native_emus::ps1
