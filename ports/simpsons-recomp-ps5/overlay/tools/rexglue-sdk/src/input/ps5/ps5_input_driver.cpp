#include <algorithm>
#include <cmath>
#include <cstring>

#include <rex/input/ps5/ps5_input_driver.h>
#include <rex/logging.h>

namespace rex::input::ps5 {
namespace {

uint32_t B(ps5rt::Button button) {
  return static_cast<uint32_t>(button);
}

int16_t Axis(float value, bool invert = false) {
  value = std::clamp(invert ? -value : value, -1.0f, 1.0f);
  const float scale = value < 0.0f ? 32768.0f : 32767.0f;
  return static_cast<int16_t>(std::lround(value * scale));
}

uint8_t Trigger(float value) {
  return static_cast<uint8_t>(
      std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}

}  // namespace

PS5InputDriver::PS5InputDriver(rex::ui::Window* window, size_t window_z_order)
    : InputDriver(window, window_z_order) {}

PS5InputDriver::~PS5InputDriver() {
  if (ready_) {
    ps5rt::shutdown_input();
  }
}

X_STATUS PS5InputDriver::Setup() {
  const auto result = ps5rt::initialize_input();
  if (!result) {
    REXLOG_ERROR("PS5 input initialization failed: {}", result.message);
    return X_STATUS_UNSUCCESSFUL;
  }
  ready_ = true;
  return X_STATUS_SUCCESS;
}

bool PS5InputDriver::Poll(ps5rt::InputSnapshot& snapshot) {
  if (!ready_) {
    return false;
  }
  const auto result = ps5rt::poll_input(snapshot);
  if (!result) {
    REXLOG_ERROR("PS5 input poll failed: {}", result.message);
    return false;
  }
  return true;
}

void PS5InputDriver::Translate(const ps5rt::ControllerState& source,
                               X_INPUT_GAMEPAD& target) {
  target = {};
  uint16_t buttons = 0;
  const uint32_t b = source.buttons;
  if (b & B(ps5rt::Button::up)) buttons |= X_INPUT_GAMEPAD_DPAD_UP;
  if (b & B(ps5rt::Button::down)) buttons |= X_INPUT_GAMEPAD_DPAD_DOWN;
  if (b & B(ps5rt::Button::left)) buttons |= X_INPUT_GAMEPAD_DPAD_LEFT;
  if (b & B(ps5rt::Button::right)) buttons |= X_INPUT_GAMEPAD_DPAD_RIGHT;
  if (b & B(ps5rt::Button::options)) buttons |= X_INPUT_GAMEPAD_START;
  // Until ps5rt exposes a separately proven Create bit, the touchpad click is
  // the deterministic Back/View mapping and avoids hijacking the PS system key.
  if (b & B(ps5rt::Button::touchpad)) buttons |= X_INPUT_GAMEPAD_BACK;
  if (b & B(ps5rt::Button::l3)) buttons |= X_INPUT_GAMEPAD_LEFT_THUMB;
  if (b & B(ps5rt::Button::r3)) buttons |= X_INPUT_GAMEPAD_RIGHT_THUMB;
  if (b & B(ps5rt::Button::l1)) buttons |= X_INPUT_GAMEPAD_LEFT_SHOULDER;
  if (b & B(ps5rt::Button::r1)) buttons |= X_INPUT_GAMEPAD_RIGHT_SHOULDER;
  if (b & B(ps5rt::Button::cross)) buttons |= X_INPUT_GAMEPAD_A;
  if (b & B(ps5rt::Button::circle)) buttons |= X_INPUT_GAMEPAD_B;
  if (b & B(ps5rt::Button::square)) buttons |= X_INPUT_GAMEPAD_X;
  if (b & B(ps5rt::Button::triangle)) buttons |= X_INPUT_GAMEPAD_Y;

  target.buttons = buttons;
  target.left_trigger = Trigger(source.l2);
  target.right_trigger = Trigger(source.r2);
  target.thumb_lx = Axis(source.left.x);
  target.thumb_ly = Axis(source.left.y, true);
  target.thumb_rx = Axis(source.right.x);
  target.thumb_ry = Axis(source.right.y, true);
}

X_RESULT PS5InputDriver::GetCapabilities(uint32_t user_index, uint32_t flags,
                                         X_INPUT_CAPABILITIES* out_caps) {
  (void)flags;
  if (user_index >= ps5rt::InputSnapshot::max_controllers) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }

  ps5rt::InputSnapshot snapshot{};
  if (!Poll(snapshot) || !snapshot.controllers[user_index].connected) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  if (!out_caps) {
    return X_ERROR_SUCCESS;
  }

  std::memset(out_caps, 0, sizeof(*out_caps));
  out_caps->type = XINPUT_DEVTYPE_GAMEPAD;
  out_caps->sub_type = 0x01;
  out_caps->flags = X_INPUT_CAPS_FFB_SUPPORTED;
  out_caps->gamepad.buttons = 0xFFFF;
  out_caps->gamepad.left_trigger = 0xFF;
  out_caps->gamepad.right_trigger = 0xFF;
  out_caps->gamepad.thumb_lx = 0x7FFF;
  out_caps->gamepad.thumb_ly = 0x7FFF;
  out_caps->gamepad.thumb_rx = 0x7FFF;
  out_caps->gamepad.thumb_ry = 0x7FFF;
  out_caps->vibration.left_motor_speed = 0xFFFF;
  out_caps->vibration.right_motor_speed = 0xFFFF;
  return X_ERROR_SUCCESS;
}

X_RESULT PS5InputDriver::GetState(uint32_t user_index, X_INPUT_STATE* out_state) {
  if (user_index >= ps5rt::InputSnapshot::max_controllers) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }

  ps5rt::InputSnapshot snapshot{};
  if (!Poll(snapshot)) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  const auto& source = snapshot.controllers[user_index];
  if (!source.connected) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }

  X_INPUT_GAMEPAD gamepad{};
  if (is_active()) {
    Translate(source, gamepad);
  }
  if (std::memcmp(&gamepad, &previous_[user_index], sizeof(gamepad)) != 0) {
    previous_[user_index] = gamepad;
    ++packet_[user_index];
  }

  if (out_state) {
    out_state->packet_number = packet_[user_index];
    out_state->gamepad = gamepad;
  }
  return X_ERROR_SUCCESS;
}

X_RESULT PS5InputDriver::SetState(uint32_t user_index,
                                  X_INPUT_VIBRATION* vibration) {
  if (user_index >= ps5rt::InputSnapshot::max_controllers || !vibration) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }

  ps5rt::InputSnapshot snapshot{};
  if (!Poll(snapshot) || !snapshot.controllers[user_index].connected) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }

  const float low = static_cast<uint16_t>(vibration->left_motor_speed) / 65535.0f;
  const float high = static_cast<uint16_t>(vibration->right_motor_speed) / 65535.0f;
  return ps5rt::set_rumble(user_index, low, high) ? X_ERROR_SUCCESS
                                                  : X_ERROR_DEVICE_NOT_CONNECTED;
}

X_RESULT PS5InputDriver::GetKeystroke(uint32_t user_index, uint32_t flags,
                                      X_INPUT_KEYSTROKE* out_keystroke) {
  (void)flags;
  (void)out_keystroke;
  if (user_index >= ps5rt::InputSnapshot::max_controllers) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  ps5rt::InputSnapshot snapshot{};
  if (!Poll(snapshot) || !snapshot.controllers[user_index].connected) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  return X_ERROR_EMPTY;
}

}  // namespace rex::input::ps5
