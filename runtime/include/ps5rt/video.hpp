#pragma once

#include <cstddef>
#include <cstdint>

#include <ps5rt/result.hpp>

namespace ps5rt {

enum class PixelFormat : std::uint8_t {
  rgb565,
  xrgb8888,
  argb8888,
  bgra8888,
  rgba8888,
};

struct VideoFrame {
  const void* pixels{};
  std::uint32_t width{};
  std::uint32_t height{};
  std::uint32_t pitch_bytes{};
  PixelFormat format{PixelFormat::xrgb8888};
  float display_aspect{};
};

struct DisplayInfo {
  std::uint32_t width{};
  std::uint32_t height{};
  double refresh_hz{};
};

class VideoDevice {
public:
  VideoDevice() = default;
  VideoDevice(const VideoDevice&) = delete;
  VideoDevice& operator=(const VideoDevice&) = delete;
  VideoDevice(VideoDevice&&) = delete;
  VideoDevice& operator=(VideoDevice&&) = delete;
  ~VideoDevice();

  Result open() noexcept;
  Result present(const VideoFrame& frame) noexcept;
  void close() noexcept;

  [[nodiscard]] bool is_open() const noexcept;
  [[nodiscard]] DisplayInfo display_info() const noexcept;

private:
  struct Impl;
  Impl* impl_{};
};

[[nodiscard]] constexpr std::size_t ps5_tiled_rgba8_offset(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t frame_width = 1920) noexcept {
  const std::uint32_t offset =
      ((y << 4) & 0x70u) ^ ((y << 5) & 0xf00u) ^
      ((y << 9) & 0x1000u) ^ ((y << 8) & 0x4000u) ^
      ((x << 2) & 0xcu) ^ ((x << 5) & 0x380u) ^
      ((x << 4) & 0x400u) ^ ((x << 6) & 0x800u) ^
      ((x << 9) & 0xa000u);
  const std::uint32_t blocks_per_row = (frame_width + 127u) >> 7;
  const std::uint32_t block_index = (y >> 7) * blocks_per_row + (x >> 7);
  return (static_cast<std::size_t>(block_index) << 16) + offset;
}

} // namespace ps5rt
