#pragma once

#include <array>
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

struct InputSnapshot {
  static constexpr std::size_t max_controllers = 4;
  std::array<ControllerState, max_controllers> controllers{};
};

Result initialize_input() noexcept;
Result poll_input(InputSnapshot& out) noexcept;
Result set_rumble(std::size_t controller, float low, float high) noexcept;
void shutdown_input() noexcept;

} // namespace ps5rt
