#pragma once

#include "vulkan_environment.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

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

  [[nodiscard]] corehost::Hooks make_hooks(
      VulkanEnvironment& vulkan,
      std::function<bool(std::uint32_t, std::uint32_t)> present_hw_frame = {});

  [[nodiscard]] bool initialized() const noexcept {
    return input_ready_ && audio_.is_open();
  }

  [[nodiscard]] bool quit_requested() const noexcept {
    return quit_requested_;
  }

  bool consume_save_state_requested() noexcept {
    const bool requested = save_state_requested_;
    save_state_requested_ = false;
    return requested;
  }

  bool consume_load_state_requested() noexcept {
    const bool requested = load_state_requested_;
    load_state_requested_ = false;
    return requested;
  }

  int consume_disc_delta_requested() noexcept {
    const int delta = disc_delta_requested_;
    disc_delta_requested_ = 0;
    return delta;
  }

  bool change_disc(int delta, std::string& error) noexcept;

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
  bool quit_requested_{};
  bool save_state_requested_{};
  bool load_state_requested_{};
  int disc_delta_requested_{};
  std::uint32_t previous_buttons_{};

  retro_set_eject_state_t disk_set_eject_state_{};
  retro_get_eject_state_t disk_get_eject_state_{};
  retro_get_image_index_t disk_get_image_index_{};
  retro_set_image_index_t disk_set_image_index_{};
  retro_get_num_images_t disk_get_num_images_{};
};

} // namespace native_emus::ps1
