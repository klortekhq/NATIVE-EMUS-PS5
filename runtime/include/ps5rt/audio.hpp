#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include <ps5rt/result.hpp>

namespace ps5rt {

enum class SampleFormat : std::uint8_t {
  s16,
  f32,
};

struct AudioSpec {
  std::uint32_t sample_rate{48000};
  std::uint32_t channels{2};
  std::uint32_t frames_per_grain{256};
  SampleFormat format{SampleFormat::f32};
};

class AudioDevice {
public:
  AudioDevice() = default;
  AudioDevice(const AudioDevice&) = delete;
  AudioDevice& operator=(const AudioDevice&) = delete;
  AudioDevice(AudioDevice&&) noexcept = default;
  AudioDevice& operator=(AudioDevice&&) noexcept = default;
  ~AudioDevice();

  Result open(const AudioSpec& requested) noexcept;
  Result write(std::span<const std::byte> interleaved_frames) noexcept;
  Result set_paused(bool paused) noexcept;
  void close() noexcept;

  [[nodiscard]] bool is_open() const noexcept;
  [[nodiscard]] AudioSpec spec() const noexcept;

private:
  struct Impl;
  Impl* impl_{};
};

} // namespace ps5rt
