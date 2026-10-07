#include <ps5rt/audio.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <pthread.h>
#include <vector>

extern "C" {
std::int32_t sceAudioOutInit();
std::int32_t sceAudioOutOpen(std::int32_t user_id, std::int32_t type, std::int32_t index,
                             std::uint32_t length, std::uint32_t frequency, std::uint32_t param);
std::int32_t sceAudioOutOutput(std::int32_t handle, const void* ptr);
std::int32_t sceAudioOutClose(std::int32_t handle);
}

namespace ps5rt {
namespace {
constexpr std::uint32_t kRate = 48000;
constexpr std::size_t kGrain = 256;
constexpr std::size_t kChannels = 2;
constexpr std::uint32_t kAlreadyInitialized = 0x8026000e;
constexpr std::size_t kWorkerStack = 2u * 1024u * 1024u;

Result error(ErrorCode code, std::int32_t native, const char* message) noexcept {
  return {code, native, message};
}
}

struct AudioDevice::Impl {
  pthread_mutex_t mutex{};
  pthread_cond_t changed{};
  pthread_t thread{};
  std::int32_t port{-1};
  std::int16_t* ring{};
  std::size_t capacity{};
  std::size_t head{};
  std::size_t count{};
  bool mutex_ready{};
  bool cond_ready{};
  bool thread_ready{};
  bool active{};
  bool shutdown{};
  bool failed{};
  AudioSpec requested{};
  std::uint64_t resample_phase{};

  static void* worker_entry(void* opaque) {
    static_cast<Impl*>(opaque)->worker();
    return nullptr;
  }

  void worker() {
    alignas(16) std::int16_t output[kGrain * kChannels]{};
    pthread_mutex_lock(&mutex);
    while (!shutdown && !failed) {
      while (!active && !shutdown)
        pthread_cond_wait(&changed, &mutex);
      if (shutdown)
        break;

      const std::size_t frames = std::min(count, kGrain);
      const std::size_t first = std::min(frames, capacity - head);
      std::memcpy(output, ring + head * kChannels,
                  first * kChannels * sizeof(std::int16_t));
      std::memcpy(output + first * kChannels, ring,
                  (frames - first) * kChannels * sizeof(std::int16_t));
      std::memset(output + frames * kChannels, 0,
                  (kGrain - frames) * kChannels * sizeof(std::int16_t));
      head = (head + frames) % capacity;
      count -= frames;
      pthread_cond_broadcast(&changed);
      pthread_mutex_unlock(&mutex);

      const int rc = sceAudioOutOutput(port, output);

      pthread_mutex_lock(&mutex);
      if (rc < 0)
        failed = true;
      pthread_cond_broadcast(&changed);
    }
    pthread_mutex_unlock(&mutex);
  }

  void reset() noexcept {
    if (mutex_ready) {
      pthread_mutex_lock(&mutex);
      shutdown = true;
      active = false;
      count = 0;
      pthread_cond_broadcast(&changed);
      pthread_mutex_unlock(&mutex);
    }
    if (thread_ready) {
      pthread_join(thread, nullptr);
      thread_ready = false;
    }
    if (port >= 0) {
      (void)sceAudioOutOutput(port, nullptr);
      (void)sceAudioOutClose(port);
      port = -1;
    }
    if (cond_ready) {
      pthread_cond_destroy(&changed);
      cond_ready = false;
    }
    if (mutex_ready) {
      pthread_mutex_destroy(&mutex);
      mutex_ready = false;
    }
    std::free(ring);
    ring = nullptr;
    capacity = head = count = 0;
    shutdown = false;
    failed = false;
    resample_phase = 0;
  }

  Result push_s16(const std::int16_t* data, std::size_t frames) noexcept {
    if (!data || !frames)
      return Result::success();
    pthread_mutex_lock(&mutex);
    std::size_t written = 0;
    while (written < frames) {
      if (failed || shutdown || !active) {
        pthread_mutex_unlock(&mutex);
        return error(ErrorCode::system_error, 0, "AudioOut worker stopped");
      }
      const std::size_t free_frames = capacity - count;
      if (!free_frames) {
        pthread_cond_wait(&changed, &mutex);
        continue;
      }
      const std::size_t n = std::min(frames - written, free_frames);
      const std::size_t tail = (head + count) % capacity;
      const std::size_t first = std::min(n, capacity - tail);
      std::memcpy(ring + tail * kChannels, data + written * kChannels,
                  first * kChannels * sizeof(std::int16_t));
      std::memcpy(ring, data + (written + first) * kChannels,
                  (n - first) * kChannels * sizeof(std::int16_t));
      count += n;
      written += n;
      pthread_cond_broadcast(&changed);
    }
    pthread_mutex_unlock(&mutex);
    return Result::success();
  }

  Result push_source_s16(const std::int16_t* data, std::size_t frames) noexcept {
    if (!data || !frames)
      return Result::success();
    if (requested.sample_rate == kRate)
      return push_s16(data, frames);

    std::vector<std::int16_t> converted;
    const std::uint64_t estimate =
        (static_cast<std::uint64_t>(frames) * kRate) / requested.sample_rate + 2;
    converted.reserve(static_cast<std::size_t>(estimate) * kChannels);

    // Exact-rate zero-order hold resampler. It deliberately favors robust
    // bring-up over DSP cleverness: no drift, no dependency, and state is
    // preserved across callback batches. A higher-quality filter can replace
    // this later without changing emulator-facing APIs.
    for (std::size_t i = 0; i < frames; ++i) {
      resample_phase += kRate;
      while (resample_phase >= requested.sample_rate) {
        converted.push_back(data[i * kChannels + 0]);
        converted.push_back(data[i * kChannels + 1]);
        resample_phase -= requested.sample_rate;
      }
    }

    return converted.empty()
        ? Result::success()
        : push_s16(converted.data(), converted.size() / kChannels);
  }
};

AudioDevice::~AudioDevice() {
  close();
  delete impl_;
}

Result AudioDevice::open(const AudioSpec& requested) noexcept {
  close();
  if (requested.channels != kChannels)
    return error(ErrorCode::unsupported, 0, "PS5 AudioOut backend requires stereo input");
  if (requested.sample_rate < 8000 || requested.sample_rate > 192000)
    return error(ErrorCode::unsupported, 0, "unsupported source sample rate");
  if (requested.frames_per_grain == 0)
    return error(ErrorCode::invalid_argument, 0, "frames_per_grain must be nonzero");

  if (!impl_)
    impl_ = new (std::nothrow) Impl();
  if (!impl_)
    return error(ErrorCode::out_of_memory, 0, "AudioDevice allocation failed");

  impl_->requested = requested;
  const std::int32_t init = sceAudioOutInit();
  if (init != 0 && static_cast<std::uint32_t>(init) != kAlreadyInitialized)
    return error(ErrorCode::system_error, init, "sceAudioOutInit failed");

  impl_->capacity = kGrain * 16;
  impl_->ring = static_cast<std::int16_t*>(
      std::calloc(impl_->capacity * kChannels, sizeof(std::int16_t)));
  if (!impl_->ring)
    return error(ErrorCode::out_of_memory, 0, "Audio ring allocation failed");

  if (pthread_mutex_init(&impl_->mutex, nullptr) != 0) {
    impl_->reset();
    return error(ErrorCode::system_error, 0, "pthread_mutex_init failed");
  }
  impl_->mutex_ready = true;
  if (pthread_cond_init(&impl_->changed, nullptr) != 0) {
    impl_->reset();
    return error(ErrorCode::system_error, 0, "pthread_cond_init failed");
  }
  impl_->cond_ready = true;

  impl_->port = sceAudioOutOpen(0xff, 0, 0, kGrain, kRate, 1);
  if (impl_->port < 0) {
    const auto rc = impl_->port;
    impl_->reset();
    return error(ErrorCode::system_error, rc, "sceAudioOutOpen failed");
  }

  impl_->active = true;
  pthread_attr_t attr;
  if (pthread_attr_init(&attr) != 0) {
    impl_->reset();
    return error(ErrorCode::system_error, 0, "pthread_attr_init failed");
  }
  (void)pthread_attr_setstacksize(&attr, kWorkerStack);
  const int tr = pthread_create(&impl_->thread, &attr, &Impl::worker_entry, impl_);
  pthread_attr_destroy(&attr);
  if (tr != 0) {
    impl_->reset();
    return error(ErrorCode::system_error, tr, "pthread_create failed");
  }
  impl_->thread_ready = true;
  return Result::success();
}

Result AudioDevice::write(std::span<const std::byte> bytes) noexcept {
  if (!impl_ || impl_->port < 0)
    return error(ErrorCode::system_error, 0, "AudioDevice is not open");

  if (impl_->requested.format == SampleFormat::s16) {
    if (bytes.size() % (sizeof(std::int16_t) * kChannels) != 0)
      return error(ErrorCode::invalid_argument, 0, "unaligned s16 stereo audio buffer");

    // The public API accepts raw bytes, so callers are not required to provide
    // storage aligned for int16_t. Copy into typed storage before decoding to
    // avoid undefined behavior on deliberately or accidentally unaligned spans.
    std::vector<std::int16_t> samples(bytes.size() / sizeof(std::int16_t));
    std::memcpy(samples.data(), bytes.data(), bytes.size());
    return impl_->push_source_s16(samples.data(), samples.size() / kChannels);
  }

  if (bytes.size() % (sizeof(float) * kChannels) != 0)
    return error(ErrorCode::invalid_argument, 0, "unaligned f32 stereo audio buffer");
  const auto frames = bytes.size() / (sizeof(float) * kChannels);
  std::vector<std::int16_t> tmp(frames * kChannels);
  for (std::size_t i = 0; i < tmp.size(); ++i) {
    float sample = 0.0f;
    std::memcpy(
        &sample,
        bytes.data() + i * sizeof(float),
        sizeof(sample));
    const float x = std::max(-1.0f, std::min(1.0f, sample));
    tmp[i] = static_cast<std::int16_t>(std::lrintf(x * 32767.0f));
  }
  return impl_->push_source_s16(tmp.data(), frames);
}

Result AudioDevice::set_paused(bool paused) noexcept {
  if (!impl_ || !impl_->mutex_ready)
    return error(ErrorCode::system_error, 0, "AudioDevice is not open");
  pthread_mutex_lock(&impl_->mutex);
  impl_->active = !paused && !impl_->failed && !impl_->shutdown;
  if (paused) {
    impl_->count = 0;
    impl_->head = 0;
  }
  pthread_cond_broadcast(&impl_->changed);
  pthread_mutex_unlock(&impl_->mutex);
  if (paused && impl_->port >= 0)
    (void)sceAudioOutOutput(impl_->port, nullptr);
  return impl_->failed
      ? error(ErrorCode::system_error, 0, "AudioOut worker failed")
      : Result::success();
}

void AudioDevice::close() noexcept {
  if (impl_)
    impl_->reset();
}

bool AudioDevice::is_open() const noexcept {
  return impl_ && impl_->port >= 0;
}

AudioSpec AudioDevice::spec() const noexcept {
  return impl_ ? impl_->requested : AudioSpec{};
}

} // namespace ps5rt
