#include <ps5rt/video.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <emmintrin.h>
#include <new>

extern "C" {
std::size_t sceKernelGetDirectMemorySize();
int sceKernelAllocateDirectMemory(std::int64_t, std::int64_t, std::size_t, std::size_t, int,
                                  std::int64_t*);
int sceKernelMapDirectMemory(void**, std::size_t, int, int, std::int64_t, std::size_t);
int sceKernelMunmap(void*, std::size_t);
int sceKernelReleaseDirectMemory(std::int64_t, std::size_t);
int sceSystemServiceHideSplashScreen();
int sceVideoOutOpen(std::int32_t, std::int32_t, std::int32_t, const void*);
int sceVideoOutClose(std::int32_t);
int sceVideoOutSetFlipRate(std::int32_t, std::int32_t);
int sceVideoOutSubmitFlip(std::int32_t, std::int32_t, std::uint32_t, std::int64_t);
int sceVideoOutWaitVblank(std::int32_t);
int sceVideoOutGetFlipStatus(std::int32_t, void*);
int sceVideoOutUnregisterBuffers(std::int32_t, std::int32_t);
}

namespace ps5rt {
namespace {
constexpr std::uint32_t kWidth = 1920;
constexpr std::uint32_t kHeight = 1080;
constexpr std::size_t kFrameBytes = 0x1000000;
constexpr std::size_t kMemoryBytes = kFrameBytes * 2;
constexpr std::size_t kAlignment = 0x200000;
constexpr int kMemoryTypeGarlicWriteCombined = 3;
constexpr int kMapProtection = 0x33;
constexpr std::uint64_t kPixelFormatRgba8Srgb = UINT64_C(0x8000000022000000);

struct VideoBuffer { void* data; void* metadata; void* reserved0; void* reserved1; };
struct VideoAttribute { std::uint8_t reserved[80]; };

extern "C" void sceVideoOutSetBufferAttribute2(
    VideoAttribute*, std::uint64_t, std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint64_t, std::uint32_t, std::uint64_t);
extern "C" int sceVideoOutRegisterBuffers2(
    std::int32_t, std::int32_t, std::int32_t, VideoBuffer*, std::int32_t,
    VideoAttribute*, std::int32_t, void*);

void flush_frame(const void* address, std::size_t bytes) noexcept {
  auto* p = static_cast<const char*>(address);
  const char* end = p + bytes;
  for (; p < end; p += 64)
    _mm_clflush(p);
  _mm_mfence();
}

std::uint32_t pixel_argb(const VideoFrame& f, std::uint32_t x, std::uint32_t y) noexcept {
  const auto* row = static_cast<const std::uint8_t*>(f.pixels) +
                    static_cast<std::size_t>(y) * f.pitch_bytes;
  if (f.format == PixelFormat::rgb1555 || f.format == PixelFormat::rgb565) {
    std::uint16_t v{};
    std::memcpy(&v, row + static_cast<std::size_t>(x) * 2, 2);
    if (f.format == PixelFormat::rgb1555) {
      const std::uint32_t r = (v >> 10) & 31u;
      const std::uint32_t g = (v >> 5) & 31u;
      const std::uint32_t b = v & 31u;
      return 0xff000000u |
             ((r << 3 | r >> 2) << 16) |
             ((g << 3 | g >> 2) << 8) |
             (b << 3 | b >> 2);
    }
    const std::uint32_t r = (v >> 11) & 31u;
    const std::uint32_t g = (v >> 5) & 63u;
    const std::uint32_t b = v & 31u;
    return 0xff000000u |
           ((r << 3 | r >> 2) << 16) |
           ((g << 2 | g >> 4) << 8) |
           (b << 3 | b >> 2);
  }

  const auto* p = row + static_cast<std::size_t>(x) * 4;
  switch (f.format) {
    case PixelFormat::xrgb8888: {
      std::uint32_t v{};
      std::memcpy(&v, p, 4);
      return 0xff000000u | (v & 0x00ffffffu);
    }
    case PixelFormat::argb8888: {
      std::uint32_t v{};
      std::memcpy(&v, p, 4);
      return v | 0xff000000u;
    }
    case PixelFormat::bgra8888:
      return 0xff000000u | (std::uint32_t(p[2]) << 16) |
             (std::uint32_t(p[1]) << 8) | p[0];
    case PixelFormat::rgba8888:
      return 0xff000000u | (std::uint32_t(p[0]) << 16) |
             (std::uint32_t(p[1]) << 8) | p[2];
    default:
      return 0xff000000u;
  }
}

} // namespace

struct VideoDevice::Impl {
  std::int32_t handle{-1};
  std::int64_t physical{-1};
  void* mapped{};
  void* frames[2]{};
  std::uint64_t flip_status[16]{};
  int back{};
  bool registered{};
  bool rect_valid[2]{};
  std::uint32_t rect_x[2]{};
  std::uint32_t rect_y[2]{};
  std::uint32_t rect_w[2]{};
  std::uint32_t rect_h[2]{};

  void write_pixel(std::uint32_t x, std::uint32_t y, std::uint32_t c) noexcept {
    if (!frames[back] || x >= kWidth || y >= kHeight)
      return;
    auto* base = static_cast<std::uint8_t*>(frames[back]);
    std::memcpy(base + ps5_tiled_rgba8_offset(x, y, kWidth), &c, sizeof(c));
  }

  void clear(std::uint32_t c) noexcept {
    if (!frames[back])
      return;
    for (std::uint32_t y = 0; y < kHeight; ++y)
      for (std::uint32_t x = 0; x < kWidth; ++x)
        write_pixel(x, y, c);
  }

  bool flip() noexcept {
    if (handle < 0 || !frames[back])
      return false;
    const int index = back;
    flush_frame(frames[index], kFrameBytes);
    if (sceVideoOutSubmitFlip(handle, index, 1, 1) < 0)
      return false;
    (void)sceVideoOutWaitVblank(handle);
    (void)sceVideoOutGetFlipStatus(handle, flip_status);
    back = 1 - index;
    return true;
  }

  void reset() noexcept {
    if (handle >= 0 && registered) {
      (void)sceVideoOutUnregisterBuffers(handle, 0);
      registered = false;
    }
    if (handle >= 0) {
      (void)sceVideoOutClose(handle);
      handle = -1;
    }
    if (mapped) {
      (void)sceKernelMunmap(mapped, kMemoryBytes);
      mapped = nullptr;
    }
    if (physical >= 0) {
      (void)sceKernelReleaseDirectMemory(physical, kMemoryBytes);
      physical = -1;
    }
    frames[0] = frames[1] = nullptr;
    back = 0;
    rect_valid[0] = rect_valid[1] = false;
  }
};

VideoDevice::~VideoDevice() {
  close();
  delete impl_;
}

Result VideoDevice::open() noexcept {
  close();
  if (!impl_)
    impl_ = new (std::nothrow) Impl();
  if (!impl_)
    return {ErrorCode::out_of_memory, 0, "VideoDevice allocation failed"};

  (void)sceSystemServiceHideSplashScreen();
  impl_->handle = sceVideoOutOpen(0xff, 0, 0, nullptr);
  if (impl_->handle < 0)
    return {ErrorCode::system_error, impl_->handle, "sceVideoOutOpen failed"};

  const std::size_t pool = sceKernelGetDirectMemorySize();
  if (pool < kMemoryBytes) {
    impl_->reset();
    return {ErrorCode::out_of_memory, 0, "insufficient direct memory for VideoOut"};
  }

  if (sceKernelAllocateDirectMemory(
          0, static_cast<std::int64_t>(pool), kMemoryBytes, kAlignment,
          kMemoryTypeGarlicWriteCombined, &impl_->physical) < 0) {
    impl_->reset();
    return {ErrorCode::system_error, 0, "sceKernelAllocateDirectMemory failed"};
  }

  void* mapped = nullptr;
  if (sceKernelMapDirectMemory(
          &mapped, kMemoryBytes, kMapProtection, 0, impl_->physical, kAlignment) < 0) {
    impl_->reset();
    return {ErrorCode::system_error, 0, "sceKernelMapDirectMemory failed"};
  }
  impl_->mapped = mapped;
  auto* base = static_cast<std::uint8_t*>(mapped);
  impl_->frames[0] = base;
  impl_->frames[1] = base + kFrameBytes;

  VideoBuffer buffers[2]{
      {impl_->frames[0], nullptr, nullptr, nullptr},
      {impl_->frames[1], nullptr, nullptr, nullptr}};
  VideoAttribute attr{};
  (void)sceVideoOutSetFlipRate(impl_->handle, 0);
  sceVideoOutSetBufferAttribute2(
      &attr, kPixelFormatRgba8Srgb, 0, kWidth, kHeight, 0, 0, 0);
  if (sceVideoOutRegisterBuffers2(
          impl_->handle, 0, 0, buffers, 2, &attr, 0, nullptr) < 0) {
    impl_->reset();
    return {ErrorCode::system_error, 0, "sceVideoOutRegisterBuffers2 failed"};
  }
  impl_->registered = true;
  impl_->back = 0;
  impl_->clear(0xff000000u);
  impl_->back = 1;
  impl_->clear(0xff000000u);
  impl_->back = 0;
  return impl_->flip()
      ? Result::success()
      : Result{ErrorCode::system_error, 0, "initial VideoOut flip failed"};
}

Result VideoDevice::present(const VideoFrame& f) noexcept {
  if (!impl_ || impl_->handle < 0)
    return {ErrorCode::system_error, 0, "VideoDevice is not open"};
  if (!f.pixels || !f.width || !f.height || !f.pitch_bytes)
    return {ErrorCode::invalid_argument, 0, "invalid video frame"};

  const double source_aspect =
      f.display_aspect > 0.0f
          ? static_cast<double>(f.display_aspect)
          : static_cast<double>(f.width) / static_cast<double>(f.height);
  const double screen_aspect = static_cast<double>(kWidth) / kHeight;

  std::uint32_t out_w = kWidth;
  std::uint32_t out_h = kHeight;
  if (source_aspect < screen_aspect)
    out_w = static_cast<std::uint32_t>(kHeight * source_aspect + 0.5);
  else if (source_aspect > screen_aspect)
    out_h = static_cast<std::uint32_t>(kWidth / source_aspect + 0.5);

  out_w = std::max<std::uint32_t>(1, std::min(out_w, kWidth));
  out_h = std::max<std::uint32_t>(1, std::min(out_h, kHeight));
  const std::uint32_t x0 = (kWidth - out_w) / 2;
  const std::uint32_t y0 = (kHeight - out_h) / 2;
  const int bi = impl_->back;

  if (!impl_->rect_valid[bi] ||
      impl_->rect_x[bi] != x0 || impl_->rect_y[bi] != y0 ||
      impl_->rect_w[bi] != out_w || impl_->rect_h[bi] != out_h) {
    impl_->clear(0xff000000u);
    impl_->rect_x[bi] = x0;
    impl_->rect_y[bi] = y0;
    impl_->rect_w[bi] = out_w;
    impl_->rect_h[bi] = out_h;
    impl_->rect_valid[bi] = true;
  }

  for (std::uint32_t oy = 0; oy < out_h; ++oy) {
    const std::uint32_t sy =
        static_cast<std::uint32_t>((std::uint64_t(oy) * f.height) / out_h);
    std::uint32_t prev_sx = UINT32_MAX;
    std::uint32_t colour = 0;
    for (std::uint32_t ox = 0; ox < out_w; ++ox) {
      const std::uint32_t sx =
          static_cast<std::uint32_t>((std::uint64_t(ox) * f.width) / out_w);
      if (sx != prev_sx) {
        prev_sx = sx;
        colour = pixel_argb(f, sx, sy);
      }
      impl_->write_pixel(x0 + ox, y0 + oy, colour);
    }
  }

  return impl_->flip()
      ? Result::success()
      : Result{ErrorCode::system_error, 0, "sceVideoOutSubmitFlip failed"};
}

void VideoDevice::close() noexcept {
  if (impl_)
    impl_->reset();
}

bool VideoDevice::is_open() const noexcept {
  return impl_ && impl_->handle >= 0;
}

DisplayInfo VideoDevice::display_info() const noexcept {
  return is_open() ? DisplayInfo{kWidth, kHeight, 60.0} : DisplayInfo{};
}

} // namespace ps5rt
