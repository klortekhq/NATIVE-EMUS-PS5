#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <ps5rt/io.hpp>
#include <ps5rt/result.hpp>

namespace ps5rt {

enum class MediaKind : std::uint8_t {
  unknown,
  cartridge,
  floppy,
  hard_disk,
  compact_disc,
  dvd,
  bluray,
};

enum class TrackKind : std::uint8_t {
  data,
  audio,
};

struct DiscTrack {
  std::uint32_t number{};
  TrackKind kind{TrackKind::data};
  std::uint32_t sector_size{2048};
  std::uint64_t first_lba{};
  std::uint64_t sectors{};
};

struct DiscLayout {
  std::vector<DiscTrack> tracks{};
  std::uint64_t total_sectors{};
};

// Host-side optical-media description used by adapters for Sega CD, Saturn,
// Dreamcast, PC Engine CD, Neo Geo CD, PC-FX, CD-i, 3DO, PS1/PS2 and others.
// CHD/CUE/CCD/MDS parsing may remain inside an emulator when upstream already
// provides the correct implementation; this contract exists so storage/network
// readers can be shared without forcing a generic disc emulator on every core.
class DiscSource {
public:
  virtual ~DiscSource() = default;

  DiscSource(const DiscSource&) = delete;
  DiscSource& operator=(const DiscSource&) = delete;

  [[nodiscard]] virtual const DiscLayout& layout() const noexcept = 0;

  virtual Result read_sector(
      std::uint64_t lba,
      std::uint32_t sector_size,
      std::span<std::byte> destination) noexcept = 0;

protected:
  DiscSource() = default;
};

using DiscSourcePtr = std::unique_ptr<DiscSource>;

// Convenience factory for adapters which want ps5rt-managed host I/O.
// Emulator-native CHD/CD parsers are free to use open_random_access() directly.
Result open_disc_source(std::string_view uri, DiscSourcePtr& out) noexcept;

} // namespace ps5rt
