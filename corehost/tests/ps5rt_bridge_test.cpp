#include <corehost/ps5rt_bridge.hpp>

#include <cassert>
#include <cstdint>

int main() {
  ps5rt::InputSnapshot snapshot{};
  auto& pad = snapshot.controllers[0];
  pad.connected = true;
  pad.buttons =
      static_cast<std::uint32_t>(ps5rt::Button::cross) |
      static_cast<std::uint32_t>(ps5rt::Button::up) |
      static_cast<std::uint32_t>(ps5rt::Button::l1);
  pad.left = {1.0f, -1.0f};
  pad.l2 = 1.0f;

  snapshot.keyboard.connected = true;
  // USB HID usage 4 = A, 40 = Return.
  snapshot.keyboard.pressed[4u >> 6u] |= std::uint64_t{1} << (4u & 63u);
  snapshot.keyboard.pressed[40u >> 6u] |= std::uint64_t{1} << (40u & 63u);

  snapshot.mouse.connected = true;
  snapshot.mouse.delta_x = 12;
  snapshot.mouse.delta_y = -9;
  snapshot.mouse.wheel_y = 1;
  snapshot.mouse.buttons = 1;

  const auto state = corehost::translate_ps5rt_input(snapshot, 0);

  assert(state.joypad_mask & (1u << 0));  // Cross -> B / south.
  assert(state.joypad_mask & (1u << 4));  // Up.
  assert(state.joypad_mask & (1u << 10)); // L1 -> L.
  assert(state.joypad_mask & (1u << 12)); // L2 digital threshold.
  assert(state.left_x == 32767);
  assert(state.left_y == -32768);
  assert(state.l2 == 32767);
  assert(state.key_down('a'));
  assert(state.key_down(13));
  assert(state.mouse_x == 12);
  assert(state.mouse_y == -9);
  assert(state.mouse_wheel_y == 1);
  assert(state.mouse_buttons == 1);

  assert(corehost::translate_pixel_format(corehost::lr::PixelFormat::rgb565) ==
         ps5rt::PixelFormat::rgb565);
  return 0;
}
