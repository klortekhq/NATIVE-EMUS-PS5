#include <corehost/ps5rt_bridge.hpp>

#include <algorithm>
#include <cmath>

namespace corehost {
namespace {

constexpr std::uint16_t lr_button(unsigned id) noexcept {
  return static_cast<std::uint16_t>(1u << id);
}

std::int16_t axis_to_i16(float value) noexcept {
  value = std::clamp(value, -1.0f, 1.0f);
  const float scaled = value < 0.0f ? value * 32768.0f : value * 32767.0f;
  return static_cast<std::int16_t>(std::lrint(scaled));
}

std::int16_t trigger_to_i16(float value) noexcept {
  value = std::clamp(value, 0.0f, 1.0f);
  return static_cast<std::int16_t>(std::lrint(value * 32767.0f));
}

void set_key(InputState& out, unsigned retro_key) noexcept {
  if (retro_key >= 512u) return;
  out.keyboard[retro_key >> 6u] |=
      (std::uint64_t{1} << (retro_key & 63u));
}

unsigned hid_to_retro(unsigned hid) noexcept {
  if (hid >= 4u && hid <= 29u) return 'a' + (hid - 4u);
  if (hid >= 30u && hid <= 38u) return '1' + (hid - 30u);
  if (hid == 39u) return '0';

  switch (hid) {
    case 40: return 13;  // Return
    case 41: return 27;  // Escape
    case 42: return 8;   // Backspace
    case 43: return 9;   // Tab
    case 44: return 32;  // Space
    case 45: return '-';
    case 46: return '=';
    case 47: return '[';
    case 48: return ']';
    case 49: return '\\';
    case 51: return ';';
    case 52: return '\'';
    case 53: return '`';
    case 54: return ',';
    case 55: return '.';
    case 56: return '/';
    case 57: return 301; // Caps Lock
    case 58: return 282; // F1
    case 59: return 283; // F2
    case 60: return 284; // F3
    case 61: return 285; // F4
    case 62: return 286; // F5
    case 63: return 287; // F6
    case 64: return 288; // F7
    case 65: return 289; // F8
    case 66: return 290; // F9
    case 67: return 291; // F10
    case 68: return 292; // F11
    case 69: return 293; // F12
    case 70: return 316; // Print Screen
    case 71: return 302; // Scroll Lock
    case 72: return 19;  // Pause
    case 73: return 277; // Insert
    case 74: return 278; // Home
    case 75: return 280; // Page Up
    case 76: return 127; // Delete
    case 77: return 279; // End
    case 78: return 281; // Page Down
    case 79: return 275; // Right
    case 80: return 276; // Left
    case 81: return 274; // Down
    case 82: return 273; // Up
    case 83: return 300; // Num Lock
    case 84: return 267; // Keypad /
    case 85: return 268; // Keypad *
    case 86: return 269; // Keypad -
    case 87: return 270; // Keypad +
    case 88: return 271; // Keypad Enter
    case 89: return 257; // Keypad 1
    case 90: return 258; // Keypad 2
    case 91: return 259; // Keypad 3
    case 92: return 260; // Keypad 4
    case 93: return 261; // Keypad 5
    case 94: return 262; // Keypad 6
    case 95: return 263; // Keypad 7
    case 96: return 264; // Keypad 8
    case 97: return 265; // Keypad 9
    case 98: return 256; // Keypad 0
    case 99: return 266; // Keypad .
    case 101: return 319; // Application/Menu
    case 103: return 272; // Keypad =
    case 224: return 306; // LCtrl
    case 225: return 304; // LShift
    case 226: return 308; // LAlt
    case 228: return 305; // RCtrl
    case 229: return 303; // RShift
    case 230: return 307; // RAlt
    default: return 0;
  }
}

bool pressed(std::uint32_t mask, ps5rt::Button b) noexcept {
  return (mask & static_cast<std::uint32_t>(b)) != 0;
}

} // namespace

InputState translate_ps5rt_input(
    const ps5rt::InputSnapshot& snapshot,
    unsigned port) noexcept {
  InputState out{};

  if (port < snapshot.controllers.size()) {
    const auto& pad = snapshot.controllers[port];
    if (pad.connected) {
      // Libretro face layout is SNES-like:
      // B=south, A=east, Y=west, X=north.
      if (pressed(pad.buttons, ps5rt::Button::cross))    out.joypad_mask |= lr_button(0);
      if (pressed(pad.buttons, ps5rt::Button::square))   out.joypad_mask |= lr_button(1);
      // Touchpad click is the verified PS5 control used as RetroPad Select.
      if (pressed(pad.buttons, ps5rt::Button::touchpad)) out.joypad_mask |= lr_button(2);
      if (pressed(pad.buttons, ps5rt::Button::options))  out.joypad_mask |= lr_button(3);
      if (pressed(pad.buttons, ps5rt::Button::up))       out.joypad_mask |= lr_button(4);
      if (pressed(pad.buttons, ps5rt::Button::down))     out.joypad_mask |= lr_button(5);
      if (pressed(pad.buttons, ps5rt::Button::left))     out.joypad_mask |= lr_button(6);
      if (pressed(pad.buttons, ps5rt::Button::right))    out.joypad_mask |= lr_button(7);
      if (pressed(pad.buttons, ps5rt::Button::circle))   out.joypad_mask |= lr_button(8);
      if (pressed(pad.buttons, ps5rt::Button::triangle)) out.joypad_mask |= lr_button(9);
      if (pressed(pad.buttons, ps5rt::Button::l1))       out.joypad_mask |= lr_button(10);
      if (pressed(pad.buttons, ps5rt::Button::r1))       out.joypad_mask |= lr_button(11);
      if (pad.l2 > 0.5f)                                 out.joypad_mask |= lr_button(12);
      if (pad.r2 > 0.5f)                                 out.joypad_mask |= lr_button(13);
      if (pressed(pad.buttons, ps5rt::Button::l3))       out.joypad_mask |= lr_button(14);
      if (pressed(pad.buttons, ps5rt::Button::r3))       out.joypad_mask |= lr_button(15);

      out.left_x = axis_to_i16(pad.left.x);
      out.left_y = axis_to_i16(pad.left.y);
      out.right_x = axis_to_i16(pad.right.x);
      out.right_y = axis_to_i16(pad.right.y);
      out.l2 = trigger_to_i16(pad.l2);
      out.r2 = trigger_to_i16(pad.r2);
    }
  }

  if (port == 0) {
    if (snapshot.keyboard.connected) {
      for (unsigned hid = 0; hid < 256u; ++hid) {
        if (!snapshot.keyboard.is_pressed(static_cast<std::uint8_t>(hid))) continue;
        const unsigned retro_key = hid_to_retro(hid);
        if (retro_key) set_key(out, retro_key);
      }
    }

    if (snapshot.mouse.connected) {
      out.mouse_x = static_cast<std::int16_t>(
          std::clamp(snapshot.mouse.delta_x, -32768, 32767));
      out.mouse_y = static_cast<std::int16_t>(
          std::clamp(snapshot.mouse.delta_y, -32768, 32767));
      out.mouse_wheel_x = static_cast<std::int16_t>(
          std::clamp(snapshot.mouse.wheel_x, -32768, 32767));
      out.mouse_wheel_y = static_cast<std::int16_t>(
          std::clamp(snapshot.mouse.wheel_y, -32768, 32767));
      out.mouse_buttons = static_cast<std::uint16_t>(
          snapshot.mouse.buttons & 0xffffu);
    }
  }

  return out;
}

ps5rt::PixelFormat translate_pixel_format(lr::PixelFormat format) noexcept {
  switch (format) {
    case lr::PixelFormat::xrgb1555: return ps5rt::PixelFormat::rgb1555;
    case lr::PixelFormat::rgb565:   return ps5rt::PixelFormat::rgb565;
    case lr::PixelFormat::xrgb8888: return ps5rt::PixelFormat::xrgb8888;
  }
  return ps5rt::PixelFormat::xrgb8888;
}

} // namespace corehost
