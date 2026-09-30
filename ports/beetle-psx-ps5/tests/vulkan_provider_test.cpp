#include "../native/vulkan_environment.hpp"
#include "../native/vulkan_provider.hpp"

#include <cassert>
#include <cstring>
#include <vector>

using native_emus::ps1::VulkanEnvironment;
using native_emus::ps1::VulkanPresentationHooks;
using native_emus::ps1::VulkanProvider;

namespace {

VkInstance fake_instance() {
  return reinterpret_cast<VkInstance>(static_cast<uintptr_t>(0x1000));
}
VkPhysicalDevice fake_gpu() {
  return reinterpret_cast<VkPhysicalDevice>(static_cast<uintptr_t>(0x2000));
}
VkDevice fake_device() {
  return reinterpret_cast<VkDevice>(static_cast<uintptr_t>(0x3000));
}
VkQueue fake_queue() {
  return reinterpret_cast<VkQueue>(static_cast<uintptr_t>(0x4000));
}
VkSurfaceKHR fake_surface() {
  return static_cast<VkSurfaceKHR>(0x5000);
}

int create_instance_calls{};
int create_device_calls{};
int destroy_instance_calls{};
int destroy_device_calls{};
int destroy_surface_calls{};
int core_destroy_device_calls{};
int core_reset_calls{};
int core_destroy_calls{};
bool instance_had_surface{};
bool instance_had_display{};
bool device_had_swapchain{};

bool has_name(const char* const* names, uint32_t count, const char* wanted) {
  for (uint32_t i=0;i<count;++i)
    if (names[i] && std::strcmp(names[i], wanted)==0) return true;
  return false;
}

VKAPI_ATTR VkResult VKAPI_CALL fake_vkCreateInstance(
    const VkInstanceCreateInfo* info,
    const VkAllocationCallbacks*,
    VkInstance* out) {
  ++create_instance_calls;
  instance_had_surface = has_name(
      info->ppEnabledExtensionNames, info->enabledExtensionCount,
      VK_KHR_SURFACE_EXTENSION_NAME);
  instance_had_display = has_name(
      info->ppEnabledExtensionNames, info->enabledExtensionCount,
      VK_KHR_DISPLAY_EXTENSION_NAME);
  *out = fake_instance();
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL fake_vkEnumeratePhysicalDevices(
    VkInstance instance, uint32_t* count, VkPhysicalDevice* devices) {
  assert(instance == fake_instance());
  if (!devices) { *count = 1; return VK_SUCCESS; }
  assert(*count >= 1);
  devices[0] = fake_gpu();
  *count = 1;
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL fake_vkCreateDevice(
    VkPhysicalDevice gpu,
    const VkDeviceCreateInfo* info,
    const VkAllocationCallbacks*,
    VkDevice* out) {
  assert(gpu == fake_gpu());
  ++create_device_calls;
  device_had_swapchain = has_name(
      info->ppEnabledExtensionNames, info->enabledExtensionCount,
      VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  *out = fake_device();
  return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL fake_vkDestroyDevice(
    VkDevice device, const VkAllocationCallbacks*) {
  assert(device == fake_device());
  ++destroy_device_calls;
}

VKAPI_ATTR void VKAPI_CALL fake_vkDestroySurfaceKHR(
    VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks*) {
  assert(instance == fake_instance());
  assert(surface == fake_surface());
  ++destroy_surface_calls;
}

VKAPI_ATTR void VKAPI_CALL fake_vkDestroyInstance(
    VkInstance instance, const VkAllocationCallbacks*) {
  assert(instance == fake_instance());
  ++destroy_instance_calls;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL fake_vkGetDeviceProcAddr(
    VkDevice, const char*) {
  return nullptr;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL fake_vkGetInstanceProcAddr(
    VkInstance, const char* name) {
#define MAP(fn) if (std::strcmp(name, #fn)==0) return reinterpret_cast<PFN_vkVoidFunction>(fake_##fn)
  MAP(vkCreateInstance);
  MAP(vkEnumeratePhysicalDevices);
  MAP(vkCreateDevice);
  MAP(vkDestroyDevice);
  MAP(vkDestroySurfaceKHR);
  MAP(vkDestroyInstance);
  MAP(vkGetDeviceProcAddr);
#undef MAP
  return nullptr;
}

const VkApplicationInfo* core_app_info() {
  static const VkApplicationInfo app{
      VK_STRUCTURE_TYPE_APPLICATION_INFO,
      nullptr,
      "BeetlePSX-test",
      1,
      "core",
      1,
      VK_API_VERSION_1_1};
  return &app;
}

VkInstance core_create_instance(
    PFN_vkGetInstanceProcAddr gip,
    const VkApplicationInfo* app,
    retro_vulkan_create_instance_wrapper_t wrapper,
    void* opaque) {
  assert(gip == fake_vkGetInstanceProcAddr);
  assert(app && app->apiVersion == VK_API_VERSION_1_1);

  const char* core_exts[] = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
  VkInstanceCreateInfo info{
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      nullptr,
      0,
      app,
      0, nullptr,
      1, core_exts};
  return wrapper(opaque, &info);
}

bool core_create_device2(
    retro_vulkan_context* context,
    VkInstance instance,
    VkPhysicalDevice gpu,
    VkSurfaceKHR surface,
    PFN_vkGetInstanceProcAddr gip,
    retro_vulkan_create_device_wrapper_t wrapper,
    void* opaque) {
  assert(instance == fake_instance());
  assert(gpu == fake_gpu());
  assert(surface == fake_surface());
  assert(gip == fake_vkGetInstanceProcAddr);

  const float priority = 1.0f;
  VkDeviceQueueCreateInfo queue{
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      nullptr, 0, 3, 1, &priority};
  VkDeviceCreateInfo info{
      VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      nullptr, 0,
      1, &queue,
      0, nullptr,
      0, nullptr,
      nullptr};

  VkDevice device = wrapper(gpu, opaque, &info);
  if (device == VK_NULL_HANDLE) return false;

  context->gpu = gpu;
  context->device = device;
  context->queue = fake_queue();
  context->queue_family_index = 3;
  context->presentation_queue = fake_queue();
  context->presentation_queue_family_index = 3;
  return true;
}

void core_destroy_device() {
  ++core_destroy_device_calls;
}

VulkanEnvironment* g_environment{};

void core_context_reset() {
  ++core_reset_calls;
  const retro_hw_render_interface* base=nullptr;
  assert(g_environment);
  assert(g_environment->environment(
      RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE, &base));
  assert(base && base->interface_type == RETRO_HW_RENDER_INTERFACE_VULKAN);
}

void core_context_destroy() {
  ++core_destroy_calls;
}

} // namespace

int main() {
  VulkanEnvironment environment;
  g_environment=&environment;

  retro_hw_render_callback hw{};
  hw.context_type=RETRO_HW_CONTEXT_VULKAN;
  hw.context_reset=core_context_reset;
  hw.context_destroy=core_context_destroy;
  assert(environment.environment(RETRO_ENVIRONMENT_SET_HW_RENDER, &hw));

  retro_hw_render_context_negotiation_interface_vulkan negotiation{};
  negotiation.interface_type=
      RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN;
  negotiation.interface_version=
      RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN_VERSION;
  negotiation.get_application_info=core_app_info;
  negotiation.create_instance=core_create_instance;
  negotiation.create_device2=core_create_device2;
  negotiation.destroy_device=core_destroy_device;
  assert(environment.environment(
      RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE,
      &negotiation));

  VulkanPresentationHooks presentation{};
  presentation.create_surface=[](VkInstance instance, VkPhysicalDevice gpu) {
    assert(instance==fake_instance());
    assert(gpu==fake_gpu());
    return fake_surface();
  };
  presentation.get_sync_index=[] { return 2u; };
  presentation.get_sync_index_mask=[] { return 0x7u; };

  VulkanProvider provider(environment, presentation);
  assert(provider.initialize(fake_vkGetInstanceProcAddr));
  assert(provider.initialized());
  assert(provider.instance()==fake_instance());
  assert(provider.gpu()==fake_gpu());
  assert(provider.device()==fake_device());
  assert(provider.surface()==fake_surface());
  assert(create_instance_calls==1);
  assert(create_device_calls==1);
  assert(instance_had_surface);
  assert(instance_had_display);
  assert(device_had_swapchain);
  assert(core_reset_calls==1);

  const retro_hw_render_interface* base=nullptr;
  assert(environment.environment(
      RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE, &base));
  const auto* iface=
      reinterpret_cast<const retro_hw_render_interface_vulkan*>(base);
  assert(iface->instance==fake_instance());
  assert(iface->gpu==fake_gpu());
  assert(iface->device==fake_device());
  assert(iface->queue==fake_queue());
  assert(iface->queue_index==3);
  assert(iface->get_sync_index(iface->handle)==2);
  assert(iface->get_sync_index_mask(iface->handle)==0x7);

  iface->lock_queue(iface->handle);
  iface->unlock_queue(iface->handle);

  provider.shutdown();
  assert(!provider.initialized());
  assert(core_destroy_calls==1);
  assert(core_destroy_device_calls==1);
  assert(destroy_device_calls==1);
  assert(destroy_surface_calls==1);
  assert(destroy_instance_calls==1);
  return 0;
}
