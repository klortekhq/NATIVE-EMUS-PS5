#pragma once

#include "vulkan_environment.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include <corehost/static_core.hpp>
#include <ps5rt/audio.hpp>
#include <ps5rt/input.hpp>

namespace native_emus::ps1 {

class RuntimeIo final {
public:
  RuntimeIo() = default;
  ~RuntimeIo();

  RuntimeIo(const RuntimeIo&) = delete;
  RuntimeIo& operator=(const RuntimeIo&) = delete;

  bool initialize(std::uint32_t source_sample_rate) noexcept;
  void shutdown() noexcept;

  [[nodiscard]] corehost::Hooks make_hooks(VulkanEnvironment& vulkan);

  [[nodiscard]] bool initialized() const noexcept {
    return input_ready_ && audio_.is_open();
  }

private:
  friend bool rumble_trampoline(
      unsigned port, enum retro_rumble_effect effect,
      std::uint16_t strength);

  bool environment(
      VulkanEnvironment& vulkan, unsigned cmd, void* data) noexcept;
  corehost::InputState input(unsigned port) noexcept;
  std::size_t audio_batch(
      const std::int16_t* samples, std::size_t frames) noexcept;
  bool set_rumble(
      unsigned port, enum retro_rumble_effect effect,
      std::uint16_t strength) noexcept;

  ps5rt::AudioDevice audio_{};
  ps5rt::InputSnapshot snapshot_{};
  std::array<float, 4> strong_rumble_{};
  std::array<float, 4> weak_rumble_{};
  bool input_ready_{};
  bool audio_error_reported_{};
};

} // namespace native_emus::ps1
