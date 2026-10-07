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
int present_calls{};
std::uint32_t next_buttons =
    static_cast<std::uint32_t>(ps5rt::Button::cross) |
    static_cast<std::uint32_t>(ps5rt::Button::options);
bool disk_ejected{};
unsigned disk_index{};
unsigned disk_count{3};
int disk_eject_calls{};
int disk_index_calls{};

bool disk_set_eject(bool ejected) {
  disk_ejected = ejected;
  ++disk_eject_calls;
  return true;
}
bool disk_get_eject() { return disk_ejected; }
unsigned disk_get_index() { return disk_index; }
bool disk_set_index(unsigned index) {
  if (index >= disk_count)
    return false;
  disk_index = index;
  ++disk_index_calls;
  return true;
}
unsigned disk_get_count() { return disk_count; }
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
  out.controllers[0].buttons = next_buttons;
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

  auto hooks = io.make_hooks(
      vulkan,
      [](std::uint32_t width, std::uint32_t height) {
        assert(width == 320);
        assert(height == 240);
        ++present_calls;
        return true;
      });

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

  hooks.video(
      RETRO_HW_FRAME_BUFFER_VALID, 320, 240, 0,
      corehost::lr::PixelFormat::xrgb8888);
  assert(present_calls == 1);

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

  unsigned disk_version = 0;
  assert(hooks.environment(
      RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION,
      &disk_version));
  assert(disk_version == 1);

  retro_disk_control_ext_callback disk_iface{
      disk_set_eject,
      disk_get_eject,
      disk_get_index,
      disk_set_index,
      disk_get_count,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
  };
  assert(hooks.environment(
      RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE,
      &disk_iface));

  std::string disk_error;
  assert(io.change_disc(1, disk_error));
  assert(disk_index == 1);
  assert(!disk_ejected);
  assert(disk_eject_calls == 2);
  assert(disk_index_calls == 1);

  assert(io.change_disc(-1, disk_error));
  assert(disk_index == 0);
  assert(!disk_ejected);
  assert(disk_eject_calls == 4);
  assert(disk_index_calls == 2);

  // Host hotkeys are rising-edge triggered: holding a combo must not perform
  // repeated state writes/loads every emulated frame.
  next_buttons =
      static_cast<std::uint32_t>(ps5rt::Button::options) |
      static_cast<std::uint32_t>(ps5rt::Button::r1);
  (void)hooks.input(0);
  assert(io.consume_save_state_requested());
  assert(!io.consume_save_state_requested());

  (void)hooks.input(0); // still held
  assert(!io.consume_save_state_requested());

  next_buttons = static_cast<std::uint32_t>(ps5rt::Button::options);
  (void)hooks.input(0); // release R1
  next_buttons =
      static_cast<std::uint32_t>(ps5rt::Button::options) |
      static_cast<std::uint32_t>(ps5rt::Button::r1);
  (void)hooks.input(0);
  assert(io.consume_save_state_requested());

  next_buttons = static_cast<std::uint32_t>(ps5rt::Button::options);
  (void)hooks.input(0);
  next_buttons =
      static_cast<std::uint32_t>(ps5rt::Button::options) |
      static_cast<std::uint32_t>(ps5rt::Button::l1);
  (void)hooks.input(0);
  assert(io.consume_load_state_requested());
  assert(!io.consume_load_state_requested());

  next_buttons = static_cast<std::uint32_t>(ps5rt::Button::options);
  (void)hooks.input(0);

  next_buttons =
      static_cast<std::uint32_t>(ps5rt::Button::touchpad) |
      static_cast<std::uint32_t>(ps5rt::Button::r1);
  (void)hooks.input(0);
  assert(io.consume_disc_delta_requested() == 1);
  assert(io.consume_disc_delta_requested() == 0);
  (void)hooks.input(0); // held
  assert(io.consume_disc_delta_requested() == 0);

  next_buttons = static_cast<std::uint32_t>(ps5rt::Button::touchpad);
  (void)hooks.input(0);
  next_buttons =
      static_cast<std::uint32_t>(ps5rt::Button::touchpad) |
      static_cast<std::uint32_t>(ps5rt::Button::l1);
  (void)hooks.input(0);
  assert(io.consume_disc_delta_requested() == -1);

  next_buttons = static_cast<std::uint32_t>(ps5rt::Button::options);
  (void)hooks.input(0);
  next_buttons =
      static_cast<std::uint32_t>(ps5rt::Button::options) |
      static_cast<std::uint32_t>(ps5rt::Button::touchpad);
  (void)hooks.input(0);
  assert(io.quit_requested());

  io.shutdown();
  assert(input_shutdown_calls == 1);
  assert(audio_close_calls == 1);

  return 0;
}
