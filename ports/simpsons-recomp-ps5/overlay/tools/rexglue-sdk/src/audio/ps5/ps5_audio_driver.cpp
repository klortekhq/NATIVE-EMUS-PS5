#include <cstddef>
#include <span>

#include <rex/assert.h>
#include <rex/audio/conversion.h>
#include <rex/audio/flags.h>
#include <rex/audio/ps5/ps5_audio_driver.h>
#include <rex/cvar.h>
#include <rex/logging.h>

REXCVAR_DEFINE_BOOL(audio_mute, false, "Audio", "Mute audio output");

namespace rex::audio::ps5 {

PS5AudioDriver::PS5AudioDriver(memory::Memory* memory,
                               rex::thread::Semaphore* semaphore)
    : AudioDriver(memory), semaphore_(semaphore) {}

PS5AudioDriver::~PS5AudioDriver() {
  Shutdown();
}

bool PS5AudioDriver::Initialize() {
  ps5rt::AudioSpec spec{};
  spec.sample_rate = kFrameFrequency;
  spec.channels = kHostChannels;
  spec.frames_per_grain = kChannelSamples;
  spec.format = ps5rt::SampleFormat::f32;

  const auto result = device_.open(spec);
  if (!result) {
    REXAPU_ERROR("PS5 AudioOut initialization failed: {}", result.message);
    return false;
  }
  ready_ = true;
  return true;
}

void PS5AudioDriver::SubmitFrame(uint32_t frame_ptr) {
  if (!ready_) {
    return;
  }

  const auto* guest = memory_->TranslateVirtual<float*>(frame_ptr);
  if (REXCVAR_GET(audio_mute)) {
    stereo_.fill(0.0f);
  } else {
    conversion::sequential_6_BE_to_interleaved_2_LE(
        stereo_.data(), guest, kChannelSamples);
  }

  const auto samples = std::span<const float>(stereo_.data(), stereo_.size());
  const auto bytes = std::as_bytes(samples);
  const auto result = device_.write(bytes);
  if (!result) {
    REXAPU_ERROR("PS5 AudioOut write failed: {}", result.message);
  }

  // AudioDevice::write copies the samples into ps5rt's native ring before
  // returning, so ReXGlue may immediately reuse the guest frame.
  if (semaphore_) {
    const auto released = semaphore_->Release(1, nullptr);
    assert_true(released);
  }
}

void PS5AudioDriver::Shutdown() {
  if (!ready_) {
    return;
  }
  device_.close();
  ready_ = false;
}

}  // namespace rex::audio::ps5
