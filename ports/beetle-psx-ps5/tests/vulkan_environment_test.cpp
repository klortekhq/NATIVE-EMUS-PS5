#include "../native/vulkan_environment.hpp"

#include <cassert>

using native_emus::ps1::VulkanEnvironment;

namespace {
VulkanEnvironment* g_env{};
int g_reset_calls{};
int g_destroy_calls{};

void core_context_reset() {
  ++g_reset_calls;
  assert(g_env);

  const retro_hw_render_interface* base = nullptr;
  assert(g_env->environment(RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE, &base));
  assert(base);
  assert(base->interface_type == RETRO_HW_RENDER_INTERFACE_VULKAN);

  const auto* vk =
      reinterpret_cast<const retro_hw_render_interface_vulkan*>(base);
  assert(vk->interface_version == RETRO_HW_RENDER_INTERFACE_VULKAN_VERSION);
}

void core_context_destroy() {
  ++g_destroy_calls;
}
} // namespace

int main() {
  VulkanEnvironment env;
  g_env = &env;

  const retro_hw_render_interface* base = nullptr;
  assert(!env.environment(RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE, &base));

  retro_hw_render_callback wrong{};
  wrong.context_type = RETRO_HW_CONTEXT_OPENGL;
  assert(!env.environment(RETRO_ENVIRONMENT_SET_HW_RENDER, &wrong));

  retro_hw_render_callback hw{};
  hw.context_type = RETRO_HW_CONTEXT_VULKAN;
  hw.context_reset = core_context_reset;
  hw.context_destroy = core_context_destroy;
  assert(env.environment(RETRO_ENVIRONMENT_SET_HW_RENDER, &hw));
  assert(env.render_requested());
  assert(!env.context_live());

  retro_hw_render_context_negotiation_interface_vulkan negotiation{};
  negotiation.interface_type =
      RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN;
  negotiation.interface_version =
      RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN_VERSION;
  assert(env.environment(
      RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE,
      &negotiation));
  assert(env.negotiation_ready());
  assert(env.negotiation());

  retro_hw_render_interface_vulkan iface{};
  iface.interface_type = RETRO_HW_RENDER_INTERFACE_VULKAN;
  iface.interface_version = RETRO_HW_RENDER_INTERFACE_VULKAN_VERSION;
  assert(env.publish_interface(iface));

  // The API must not be visible before the frontend calls context_reset.
  assert(!env.environment(RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE, &base));

  assert(env.context_reset());
  assert(g_reset_calls == 1);
  assert(env.context_live());

  base = nullptr;
  assert(env.environment(RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE, &base));
  assert(base);

  env.context_destroy();
  assert(g_destroy_calls == 1);
  assert(!env.context_live());
  assert(!env.environment(RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE, &base));

  env.clear();
  assert(!env.render_requested());
  assert(!env.negotiation_ready());
  return 0;
}
