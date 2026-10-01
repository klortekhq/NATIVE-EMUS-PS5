#include <ps5rt/media.hpp>

#include <array>
#include <cassert>
#include <cstdint>

int main() {
  using ps5rt::DiscSectorMode;
  using ps5rt::DiscTrack;

  static_assert(ps5rt::native_sector_size(DiscSectorMode::mode1_2048) == 2048);
  static_assert(ps5rt::native_sector_size(DiscSectorMode::mode2_2336) == 2336);
  static_assert(ps5rt::native_sector_size(DiscSectorMode::mode1_2352) == 2352);
  static_assert(ps5rt::native_sector_size(DiscSectorMode::mode2_2352) == 2352);
  static_assert(ps5rt::native_sector_size(DiscSectorMode::audio_2352) == 2352);

  // Typical PS1 mixed-mode layout: a raw data track followed by CD audio.
  constexpr std::array<DiscTrack, 2> mixed_mode{{
      {1, ps5rt::TrackKind::data, DiscSectorMode::mode2_2352, 2352, 0, 15000},
      {2, ps5rt::TrackKind::audio, DiscSectorMode::audio_2352, 2352, 15000, 12000},
  }};
  static_assert(mixed_mode[0].has_valid_sector_size());
  static_assert(mixed_mode[1].has_valid_sector_size());

  // Cooked ISO and Mode2/2336 are both valid source representations.
  constexpr DiscTrack cooked{
      1, ps5rt::TrackKind::data, DiscSectorMode::mode1_2048, 2048, 0, 1000};
  constexpr DiscTrack mode2{
      1, ps5rt::TrackKind::data, DiscSectorMode::mode2_2336, 2336, 0, 1000};
  static_assert(cooked.has_valid_sector_size());
  static_assert(mode2.has_valid_sector_size());

  // Regression: never silently call a 2048-byte track a raw 2352-byte track.
  constexpr DiscTrack invalid{
      1, ps5rt::TrackKind::data, DiscSectorMode::mode1_2048, 2352, 0, 1000};
  static_assert(!invalid.has_valid_sector_size());

  // Unknown mode may preserve one of the known physical widths until the
  // emulator-specific parser resolves the exact track mode.
  static_assert(ps5rt::valid_native_sector_size(DiscSectorMode::unknown, 2048));
  static_assert(ps5rt::valid_native_sector_size(DiscSectorMode::unknown, 2336));
  static_assert(ps5rt::valid_native_sector_size(DiscSectorMode::unknown, 2352));
  static_assert(!ps5rt::valid_native_sector_size(DiscSectorMode::unknown, 4096));

  return 0;
}
