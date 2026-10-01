#pragma once

#include <rex/audio/audio_system.h>

namespace rex::audio::ps5 {

class PS5AudioSystem final : public AudioSystem {
 public:
  explicit PS5AudioSystem(runtime::FunctionDispatcher* function_dispatcher);
  ~PS5AudioSystem() override = default;

  static bool IsAvailable() { return true; }

  X_STATUS CreateDriver(size_t index, rex::thread::Semaphore* semaphore,
                        AudioDriver** out_driver) override;
  void DestroyDriver(AudioDriver* driver) override;
};

}  // namespace rex::audio::ps5
