#include <ps5rt/audio.hpp>
#include <ps5rt/result.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace {

std::mutex g_mutex;
int g_init_calls = 0;
int g_open_calls = 0;
int g_close_calls = 0;
int g_flush_calls = 0;
std::vector<std::int16_t> g_samples;

}  // namespace

extern "C" {

std::int32_t sceAudioOutInit() {
  ++g_init_calls;
  return 0;
}

std::int32_t sceAudioOutOpen(
    std::int32_t user_id,
    std::int32_t type,
    std::int32_t index,
    std::uint32_t length,
    std::uint32_t frequency,
    std::uint32_t param) {
  assert(user_id == 0xff);
  assert(type == 0);
  assert(index == 0);
  assert(length == 256);
  assert(frequency == 48000);
  assert(param == 1);
  ++g_open_calls;
  return 7;
}

std::int32_t sceAudioOutOutput(
    std::int32_t handle,
    const void* ptr) {
  assert(handle == 7);
  if (ptr == nullptr) {
    ++g_flush_calls;
    return 0;
  }

  const auto* samples =
      static_cast<const std::int16_t*>(ptr);
  std::lock_guard<std::mutex> lock(g_mutex);
  g_samples.insert(
      g_samples.end(),
      samples,
      samples + 256 * 2);
  return 0;
}

std::int32_t sceAudioOutClose(std::int32_t handle) {
  assert(handle == 7);
  ++g_close_calls;
  return 0;
}

}  // extern "C"

namespace {

bool wait_for_samples(std::size_t count) {
  for (int attempt = 0; attempt < 500; ++attempt) {
    {
      std::lock_guard<std::mutex> lock(g_mutex);
      if (g_samples.size() >= count)
        return true;
    }
    std::this_thread::sleep_for(
        std::chrono::milliseconds(1));
  }
  return false;
}

void clear_samples() {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_samples.clear();
}

}  // namespace

int main() {
  using namespace ps5rt;

  AudioDevice audio;

  AudioSpec bad_channels{};
  bad_channels.channels = 1;
  auto result = audio.open(bad_channels);
  assert(!result);
  assert(result.code == ErrorCode::unsupported);
  assert(!audio.is_open());

  AudioSpec bad_grain{};
  bad_grain.frames_per_grain = 0;
  result = audio.open(bad_grain);
  assert(!result);
  assert(result.code == ErrorCode::invalid_argument);

  AudioSpec spec{};
  spec.sample_rate = 48000;
  spec.channels = 2;
  spec.frames_per_grain = 64;
  spec.format = SampleFormat::f32;
  result = audio.open(spec);
  assert(result);
  assert(audio.is_open());
  assert(audio.spec().sample_rate == 48000);
  assert(g_init_calls == 1);
  assert(g_open_calls == 1);

  std::array<float, 8> source{
      -2.0F, -1.0F,
      -0.5F, 0.0F,
      0.5F, 1.0F,
      2.0F, 0.25F,
  };
  result = audio.write(std::as_bytes(std::span{source}));
  assert(result);
  assert(wait_for_samples(256 * 2));

  {
    std::lock_guard<std::mutex> lock(g_mutex);
    assert(g_samples[0] == -32767);
    assert(g_samples[1] == -32767);
    assert(g_samples[2] == -16384);
    assert(g_samples[3] == 0);
    assert(g_samples[4] == 16384);
    assert(g_samples[5] == 32767);
    assert(g_samples[6] == 32767);
    assert(
        std::abs(
            static_cast<int>(g_samples[7]) -
            8192) <= 1);
  }

  result = audio.set_paused(true);
  assert(result);
  assert(g_flush_calls >= 1);

  result = audio.set_paused(false);
  assert(result);

  audio.close();
  assert(!audio.is_open());
  assert(g_close_calls == 1);

  clear_samples();

  AudioSpec resampled{};
  resampled.sample_rate = 24000;
  resampled.channels = 2;
  resampled.frames_per_grain = 32;
  resampled.format = SampleFormat::s16;
  result = audio.open(resampled);
  assert(result);

  const std::array<std::int16_t, 4> s16{
      100, -100,
      200, -200,
  };
  result = audio.write(std::as_bytes(std::span{s16}));
  assert(result);
  assert(wait_for_samples(256 * 2));

  {
    std::lock_guard<std::mutex> lock(g_mutex);
    assert(g_samples[0] == 100);
    assert(g_samples[1] == -100);
    assert(g_samples[2] == 100);
    assert(g_samples[3] == -100);
    assert(g_samples[4] == 200);
    assert(g_samples[5] == -200);
    assert(g_samples[6] == 200);
    assert(g_samples[7] == -200);
  }

  audio.close();
  assert(g_close_calls == 2);

  std::cout << "PS5 AudioOut host-mock tests passed\n";
  return 0;
}
