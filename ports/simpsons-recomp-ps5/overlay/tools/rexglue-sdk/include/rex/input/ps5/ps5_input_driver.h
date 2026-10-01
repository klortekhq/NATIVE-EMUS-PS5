#pragma once

#include <array>
#include <cstdint>

#include <ps5rt/input.hpp>
#include <rex/input/input_driver.h>

namespace rex::input::ps5 {

class PS5InputDriver final : public InputDriver {
 public:
  PS5InputDriver(rex::ui::Window* window, size_t window_z_order);
  ~PS5InputDriver() override;

  X_STATUS Setup() override;
  X_RESULT GetCapabilities(uint32_t user_index, uint32_t flags,
                           X_INPUT_CAPABILITIES* out_caps) override;
  X_RESULT GetState(uint32_t user_index, X_INPUT_STATE* out_state) override;
  X_RESULT SetState(uint32_t user_index, X_INPUT_VIBRATION* vibration) override;
  X_RESULT GetKeystroke(uint32_t user_index, uint32_t flags,
                        X_INPUT_KEYSTROKE* out_keystroke) override;

 private:
  bool Poll(ps5rt::InputSnapshot& snapshot);
  static void Translate(const ps5rt::ControllerState& source, X_INPUT_GAMEPAD& target);

  std::array<X_INPUT_GAMEPAD, ps5rt::InputSnapshot::max_controllers> previous_{};
  std::array<uint32_t, ps5rt::InputSnapshot::max_controllers> packet_{};
  bool ready_{};
};

}  // namespace rex::input::ps5
