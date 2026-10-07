#pragma once

#include <cstdint>
#include <memory>
#include <span>
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

// Physical/native sector representation as stored by the backing image.
//
// Do not collapse these to one "PS1 sector size". Real/mixed-mode images may
// store tracks with different native sector widths. The emulator may later
// decode/canonicalize a sector, but the host/VFS layer must preserve the
// source representation requested by the core.
enum class DiscSectorMode : std::uint8_t {
  unknown,
  audio_2352,
  mode1_2048,
  mode1_2352,
  mode2_2336,
  mode2_2352,
};

[[nodiscard]] constexpr std::uint32_t native_sector_size(
    DiscSectorMode mode) noexcept {
  switch (mode) {
    case DiscSectorMode::audio_2352:
    case DiscSectorMode::mode1_2352:
    case DiscSectorMode::mode2_2352:
      return 2352;
    case DiscSectorMode::mode2_2336:
      return 2336;
    case DiscSectorMode::mode1_2048:
      return 2048;
    case DiscSectorMode::unknown:
      return 0;
  }
  return 0;
}

[[nodiscard]] constexpr bool valid_native_sector_size(
    DiscSectorMode mode,
    std::uint32_t bytes) noexcept {
  const auto expected = native_sector_size(mode);
  return mode == DiscSectorMode::unknown
      ? (bytes == 2048 || bytes == 2336 || bytes == 2352)
      : bytes == expected;
}

struct DiscTrack {
  std::uint32_t number{};
  TrackKind kind{TrackKind::data};
  DiscSectorMode mode{DiscSectorMode::mode1_2048};
  std::uint32_t sector_size{2048};
  std::uint64_t first_lba{};
  std::uint64_t sectors{};

  [[nodiscard]] constexpr bool has_valid_sector_size() const noexcept {
    return valid_native_sector_size(mode, sector_size);
  }
};

struct DiscLayout {
  std::vector<DiscTrack> tracks{};
  std::uint64_t total_sectors{};
};

[[nodiscard]] inline const DiscTrack* find_track_for_lba(
    const DiscLayout& layout,
    std::uint64_t lba) noexcept {
  for (const auto& track : layout.tracks) {
    const auto end = track.first_lba + track.sectors;
    if (lba >= track.first_lba && lba < end)
      return &track;
  }
  return nullptr;
}

[[nodiscard]] inline std::uint32_t native_sector_size_for_lba(
    const DiscLayout& layout,
    std::uint64_t lba) noexcept {
  const auto* track = find_track_for_lba(layout, lba);
  return track && track->has_valid_sector_size() ? track->sector_size : 0;
}

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
