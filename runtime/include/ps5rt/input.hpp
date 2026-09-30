#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <ps5rt/result.hpp>

namespace ps5rt {

enum class Button : std::uint32_t {
  up        = 1u << 0,
  down      = 1u << 1,
  left      = 1u << 2,
  right     = 1u << 3,
  cross     = 1u << 4,
  circle    = 1u << 5,
  square    = 1u << 6,
  triangle  = 1u << 7,
  l1        = 1u << 8,
  r1        = 1u << 9,
  l3        = 1u << 10,
  r3        = 1u << 11,
  options   = 1u << 12,
  create    = 1u << 13,
  touchpad  = 1u << 14,
};

struct Stick {
  float x{};
  float y{};
};

struct ControllerState {
  bool connected{};
  std::uint32_t user_id{};
  std::uint32_t buttons{};
  Stick left{};
  Stick right{};
  float l2{};
  float r2{};
};

// USB HID keyboard usage IDs 0..255. Emulator-specific keyboard matrices stay
// in the emulator; ps5rt only reports host key state.
struct KeyboardState {
  bool connected{};
  std::array<std::uint64_t, 4> pressed{}; // 256 HID usages

  [[nodiscard]] constexpr bool is_pressed(std::uint8_t usage) const noexcept {
    const auto word = static_cast<std::size_t>(usage >> 6);
    const auto bit = static_cast<std::uint64_t>(usage & 63u);
    return (pressed[word] & (std::uint64_t{1} << bit)) != 0;
  }
};

struct MouseState {
  bool connected{};
  std::int32_t delta_x{};
  std::int32_t delta_y{};
  std::int32_t wheel_x{};
  std::int32_t wheel_y{};
  std::uint32_t buttons{};
};

struct InputSnapshot {
  static constexpr std::size_t max_controllers = 4;
  std::array<ControllerState, max_controllers> controllers{};
  KeyboardState keyboard{};
  MouseState mouse{};
};

Result initialize_input() noexcept;
Result poll_input(InputSnapshot& out) noexcept;
Result set_rumble(std::size_t controller, float low, float high) noexcept;
void shutdown_input() noexcept;

} // namespace ps5rt
