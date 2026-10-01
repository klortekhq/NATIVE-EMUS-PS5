#pragma once

#include <array>

#include <ps5rt/audio.hpp>
#include <rex/audio/audio_driver.h>
#include <rex/thread.h>

namespace rex::audio::ps5 {

class PS5AudioDriver final : public AudioDriver {
 public:
  PS5AudioDriver(memory::Memory* memory, rex::thread::Semaphore* semaphore);
  ~PS5AudioDriver() override;

  bool Initialize();
  void SubmitFrame(uint32_t frame_ptr) override;
  void Shutdown();

 private:
  static constexpr uint32_t kFrameFrequency = 48000;
  static constexpr uint32_t kGuestChannels = 6;
  static constexpr uint32_t kHostChannels = 2;
  static constexpr uint32_t kChannelSamples = 256;

  rex::thread::Semaphore* semaphore_{};
  ps5rt::AudioDevice device_;
  std::array<float, kChannelSamples * kHostChannels> stereo_{};
  bool ready_{};
};

}  // namespace rex::audio::ps5
