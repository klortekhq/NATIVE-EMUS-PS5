#include "../native/runtime_io.hpp"
#include <ps5rt/log.hpp>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace {
int input_init_calls{};
int input_shutdown_calls{};
int input_poll_calls{};
int audio_open_calls{};
int audio_close_calls{};
int audio_write_calls{};
std::size_t audio_bytes{};
float last_low{};
float last_high{};
int rumble_calls{};
}

namespace ps5rt {

struct AudioDevice::Impl {};

AudioDevice::~AudioDevice() = default;

Result AudioDevice::open(const AudioSpec& spec) noexcept {
  ++audio_open_calls;
  assert(spec.sample_rate == 44100);
  assert(spec.channels == 2);
  assert(spec.format == SampleFormat::s16);
  impl_ = reinterpret_cast<Impl*>(static_cast<std::uintptr_t>(1));
  return Result::success();
}

Result AudioDevice::write(std::span<const std::byte> bytes) noexcept {
  ++audio_write_calls;
  audio_bytes += bytes.size();
  return Result::success();
}

Result AudioDevice::set_paused(bool) noexcept {
  return Result::success();
}

void AudioDevice::close() noexcept {
  if (impl_)
    ++audio_close_calls;
  impl_ = nullptr;
}

bool AudioDevice::is_open() const noexcept {
  return impl_ != nullptr;
}

AudioSpec AudioDevice::spec() const noexcept {
  AudioSpec spec{};
  spec.sample_rate = 44100;
  spec.channels = 2;
  spec.format = SampleFormat::s16;
  return spec;
}

Result initialize_input() noexcept {
  ++input_init_calls;
  return Result::success();
}

Result poll_input(InputSnapshot& out) noexcept {
  ++input_poll_calls;
  out = {};
  out.controllers[0].connected = true;
  out.controllers[0].buttons =
      static_cast<std::uint32_t>(Button::cross) |
      static_cast<std::uint32_t>(Button::options);
  out.controllers[0].left = {0.5f, -0.5f};
  return Result::success();
}

Result set_rumble(std::size_t controller, float low, float high) noexcept {
  assert(controller == 0);
  ++rumble_calls;
  last_low = low;
  last_high = high;
  return Result::success();
}

void shutdown_input() noexcept {
  ++input_shutdown_calls;
}

void log(LogLevel, std::string_view, std::string_view) noexcept {}

} // namespace ps5rt

int main() {
  native_emus::ps1::VulkanEnvironment vulkan;
  native_emus::ps1::RuntimeIo io;

  assert(io.initialize(44100));
  assert(io.initialized());
  assert(input_init_calls == 1);
  assert(audio_open_calls == 1);

  auto hooks = io.make_hooks(vulkan);

  const auto p0 = hooks.input(0);
  const auto p1 = hooks.input(1);
  assert(input_poll_calls == 1);
  assert((p0.joypad_mask & (1u << 0)) != 0); // Cross -> south/B.
  assert((p0.joypad_mask & (1u << 3)) != 0); // Options -> Start.
  assert(p0.left_x > 16000 && p0.left_x < 16500);
  assert(p1.joypad_mask == 0);

  std::int16_t samples[8]{};
  assert(hooks.audio_batch(samples, 4) == 4);
  assert(audio_write_calls == 1);
  assert(audio_bytes == sizeof(samples));

  retro_rumble_interface rumble{};
  assert(hooks.environment(
      RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble));
  assert(rumble.set_rumble_state);

  assert(rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, 0xffff));
  assert(std::fabs(last_low - 1.0f) < 0.001f);
  assert(std::fabs(last_high - 0.0f) < 0.001f);

  assert(rumble.set_rumble_state(0, RETRO_RUMBLE_WEAK, 0x8000));
  assert(std::fabs(last_low - 1.0f) < 0.001f);
  assert(last_high > 0.49f && last_high < 0.51f);
  assert(rumble_calls == 2);

  io.shutdown();
  assert(input_shutdown_calls == 1);
  assert(audio_close_calls == 1);

  return 0;
}
