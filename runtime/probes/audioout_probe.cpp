#include <ps5rt/audio.hpp>

#include <array>
#include <cstddef>
#include <cstdio>
#include <span>

namespace {

constexpr std::uint32_t kSampleRate = 48000;
constexpr std::uint32_t kFramesPerGrain = 256;
constexpr std::size_t kChannels = 2;
constexpr std::size_t kGrains = 280; // ~1.49 s; exceeds the 16-grain ring many times.

std::array<float, kFramesPerGrain * kChannels>
make_square_grain(std::size_t grain_index) noexcept {
  std::array<float, kFramesPerGrain * kChannels> samples{};
  constexpr std::size_t half_period = 30; // ~800 Hz at 48 kHz.
  const std::size_t base = grain_index * kFramesPerGrain;
  for (std::size_t frame = 0; frame < kFramesPerGrain; ++frame) {
    const bool high = ((base + frame) / half_period) % 2u == 0u;
    const float value = high ? 0.12f : -0.12f;
    samples[frame * 2u] = value;
    samples[frame * 2u + 1u] = value;
  }
  return samples;
}

} // namespace

int main() {
  std::printf("ps5rt-audioout-probe: start\n");
  std::fflush(stdout);

  ps5rt::AudioSpec spec{};
  spec.sample_rate = kSampleRate;
  spec.channels = 2;
  spec.frames_per_grain = kFramesPerGrain;
  spec.format = ps5rt::SampleFormat::f32;

  ps5rt::AudioDevice audio;
  const auto opened = audio.open(spec);
  if (!opened) {
    std::printf(
        "ps5rt-audioout-probe: open FAILED rc=%d\n",
        static_cast<int>(opened.native_code));
    std::fflush(stdout);
    return 1;
  }

  for (std::size_t grain = 0; grain < kGrains; ++grain) {
    const auto samples = make_square_grain(grain);
    const auto bytes = std::as_bytes(std::span{samples});
    const auto wrote = audio.write(bytes);
    if (!wrote) {
      std::printf(
          "ps5rt-audioout-probe: write FAILED grain=%zu rc=%d\n",
          grain,
          static_cast<int>(wrote.native_code));
      std::fflush(stdout);
      return 2;
    }
  }

  const auto paused = audio.set_paused(true);
  if (!paused) {
    std::printf(
        "ps5rt-audioout-probe: pause FAILED rc=%d\n",
        static_cast<int>(paused.native_code));
    std::fflush(stdout);
    return 3;
  }

  audio.close();
  std::printf(
      "ps5rt-audioout-probe: PASS grains=%zu frames=%zu\n",
      kGrains,
      kGrains * static_cast<std::size_t>(kFramesPerGrain));
  std::fflush(stdout);
  return 0;
}
