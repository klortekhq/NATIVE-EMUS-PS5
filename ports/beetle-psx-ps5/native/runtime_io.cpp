#include "runtime_io.hpp"

#include <corehost/ps5rt_bridge.hpp>
#include <ps5rt/log.hpp>

#include <algorithm>
#include <span>
#include <string_view>
#include <utility>

namespace native_emus::ps1 {
namespace {
RuntimeIo* g_active_io = nullptr;

float rumble_strength(std::uint16_t strength) noexcept {
  return static_cast<float>(strength) / 65535.0f;
}
} // namespace

bool rumble_trampoline(
    unsigned port,
    enum retro_rumble_effect effect,
    std::uint16_t strength) {
  return g_active_io
      ? g_active_io->set_rumble(port, effect, strength)
      : false;
}

RuntimeIo::~RuntimeIo() {
  shutdown();
}

bool RuntimeIo::initialize(std::uint32_t source_sample_rate) noexcept {
  shutdown();

  if (source_sample_rate < 8000 || source_sample_rate > 192000)
    return false;
  if (g_active_io && g_active_io != this)
    return false;

  const auto input_result = ps5rt::initialize_input();
  if (!input_result)
    return false;
  input_ready_ = true;

  ps5rt::AudioSpec spec{};
  spec.sample_rate = source_sample_rate;
  spec.channels = 2;
  spec.frames_per_grain = 256;
  spec.format = ps5rt::SampleFormat::s16;

  const auto audio_result = audio_.open(spec);
  if (!audio_result) {
    ps5rt::shutdown_input();
    input_ready_ = false;
    return false;
  }

  g_active_io = this;
  audio_error_reported_ = false;
  return true;
}

void RuntimeIo::shutdown() noexcept {
  if (g_active_io == this)
    g_active_io = nullptr;

  for (std::size_t port = 0; port < strong_rumble_.size(); ++port) {
    if ((strong_rumble_[port] != 0.0f || weak_rumble_[port] != 0.0f) &&
        input_ready_) {
      (void)ps5rt::set_rumble(port, 0.0f, 0.0f);
    }
  }
  strong_rumble_.fill(0.0f);
  weak_rumble_.fill(0.0f);

  audio_.close();

  if (input_ready_) {
    ps5rt::shutdown_input();
    input_ready_ = false;
  }

  snapshot_ = {};
  audio_error_reported_ = false;
}

corehost::Hooks RuntimeIo::make_hooks(
    VulkanEnvironment& vulkan,
    std::function<bool(std::uint32_t, std::uint32_t)> present_hw_frame) {
  corehost::Hooks hooks{};

  hooks.audio_batch = [this](
      const std::int16_t* samples, std::size_t frames) {
    return audio_batch(samples, frames);
  };

  hooks.input = [this](unsigned port) {
    return input(port);
  };

  hooks.log = [](std::string_view message) {
    ps5rt::log(ps5rt::LogLevel::info, "ps1", message);
  };

  hooks.environment = [this, &vulkan](unsigned cmd, void* data) {
    return environment(vulkan, cmd, data);
  };

  hooks.video = [present_hw_frame = std::move(present_hw_frame)](
      const void* data,
      unsigned width,
      unsigned height,
      std::size_t,
      corehost::lr::PixelFormat) {
    if (data != RETRO_HW_FRAME_BUFFER_VALID || !present_hw_frame)
      return;
    if (!present_hw_frame(width, height)) {
      ps5rt::log(
          ps5rt::LogLevel::error, "ps1",
          "Vulkan presenter rejected hardware frame");
    }
  };

  return hooks;
}

bool RuntimeIo::environment(
    VulkanEnvironment& vulkan,
    unsigned cmd,
    void* data) noexcept {
  if (cmd == RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE) {
    if (!data)
      return false;
    auto* iface = static_cast<retro_rumble_interface*>(data);
    iface->set_rumble_state = &rumble_trampoline;
    return true;
  }

  return vulkan.environment(cmd, data);
}

corehost::InputState RuntimeIo::input(unsigned port) noexcept {
  if (!input_ready_)
    return {};

  // corehost requests ports in ascending order after one libretro input_poll().
  // Refresh once on port zero, then translate the same coherent snapshot for
  // the remaining controller ports.
  if (port == 0) {
    const auto result = ps5rt::poll_input(snapshot_);
    if (!result) {
      snapshot_ = {};
      ps5rt::log(
          ps5rt::LogLevel::warning, "ps1",
          "DualSense poll failed; returning neutral input");
    }
  }

  return corehost::translate_ps5rt_input(snapshot_, port);
}

std::size_t RuntimeIo::audio_batch(
    const std::int16_t* samples,
    std::size_t frames) noexcept {
  if (!samples || frames == 0 || !audio_.is_open())
    return frames;

  const auto* bytes = reinterpret_cast<const std::byte*>(samples);
  const auto byte_count =
      frames * 2u * sizeof(std::int16_t);

  const auto result = audio_.write(
      std::span<const std::byte>(bytes, byte_count));

  if (!result && !audio_error_reported_) {
    audio_error_reported_ = true;
    ps5rt::log(
        ps5rt::LogLevel::error, "ps1",
        "AudioOut write failed; dropping audio until the backend recovers");
  }

  // Libretro audio callbacks are push-only. Returning the frame count even on
  // a host-output failure prevents the emulation thread from retrying the same
  // samples forever.
  return frames;
}

bool RuntimeIo::set_rumble(
    unsigned port,
    enum retro_rumble_effect effect,
    std::uint16_t strength) noexcept {
  if (!input_ready_ || port >= strong_rumble_.size())
    return false;

  const float value = rumble_strength(strength);
  switch (effect) {
    case RETRO_RUMBLE_STRONG:
      strong_rumble_[port] = value;
      break;
    case RETRO_RUMBLE_WEAK:
      weak_rumble_[port] = value;
      break;
    default:
      return false;
  }

  return static_cast<bool>(ps5rt::set_rumble(
      port, strong_rumble_[port], weak_rumble_[port]));
}

} // namespace native_emus::ps1
