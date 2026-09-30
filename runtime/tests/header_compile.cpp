#include <ps5rt/ps5rt.hpp>

#include <type_traits>

static_assert(std::is_trivially_copyable_v<ps5rt::Result>);
static_assert(sizeof(ps5rt::ControllerState) > 0);
static_assert(sizeof(ps5rt::MemoryRequest) > 0);
static_assert(sizeof(ps5rt::JitRequest) > 0);
static_assert(sizeof(ps5rt::KeyboardState) > 0);
static_assert(sizeof(ps5rt::DiscTrack) > 0);

int main() {
  ps5rt::MemoryRequest request{};
  request.size = 64 * 1024;
  request.alignment = 64 * 1024;

  ps5rt::AudioSpec audio{};
  audio.sample_rate = 48000;
  audio.channels = 2;

  ps5rt::VulkanConfig vk{};
  vk.enable_shader_cache = true;

  return (request.size && audio.sample_rate && vk.enable_shader_cache) ? 0 : 1;
}
