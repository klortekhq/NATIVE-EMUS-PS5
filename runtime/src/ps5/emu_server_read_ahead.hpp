#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace ps5rt::detail {

class EmuServerReadAheadPolicy {
public:
  EmuServerReadAheadPolicy(
      std::size_t base_bytes,
      std::size_t max_bytes) noexcept
      : base_bytes_(base_bytes),
        max_bytes_(max_bytes == 0
                       ? base_bytes
                       : std::max(base_bytes, max_bytes)),
        window_bytes_(base_bytes) {}

  void reset() noexcept {
    window_bytes_ = base_bytes_;
    previous_end_ = 0;
    has_previous_ = false;
  }

  [[nodiscard]] std::size_t suggest(
      std::uint64_t offset,
      std::size_t requested,
      std::size_t remaining) noexcept {
    const auto bounded_request = std::min(requested, remaining);
    if (bounded_request == 0)
      return 0;

    const bool sequential =
        has_previous_ && offset == previous_end_;

    if (base_bytes_ == 0) {
      window_bytes_ = 0;
    } else if (!sequential) {
      window_bytes_ = base_bytes_;
    } else if (window_bytes_ < max_bytes_) {
      const auto room = max_bytes_ - window_bytes_;
      window_bytes_ += std::min(window_bytes_, room);
    }

    const auto end_delta =
        static_cast<std::uint64_t>(bounded_request);
    previous_end_ =
        offset > std::numeric_limits<std::uint64_t>::max() - end_delta
            ? std::numeric_limits<std::uint64_t>::max()
            : offset + end_delta;
    has_previous_ = true;

    return std::min(
        remaining,
        std::max(bounded_request, window_bytes_));
  }

  [[nodiscard]] std::size_t base_bytes() const noexcept {
    return base_bytes_;
  }

  [[nodiscard]] std::size_t max_bytes() const noexcept {
    return max_bytes_;
  }

private:
  std::size_t base_bytes_{};
  std::size_t max_bytes_{};
  std::size_t window_bytes_{};
  std::uint64_t previous_end_{};
  bool has_previous_{};
};

} // namespace ps5rt::detail
