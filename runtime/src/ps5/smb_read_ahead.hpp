#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <utility>
#include <vector>

namespace ps5rt::detail {

class SmbReadAheadCache {
public:
  explicit SmbReadAheadCache(std::size_t capacity_bytes) noexcept
      : capacity_bytes_(capacity_bytes) {}

  void clear() noexcept {
    data_.clear();
    offset_ = 0;
  }

  [[nodiscard]] std::size_t capacity_bytes() const noexcept {
    return capacity_bytes_;
  }

  [[nodiscard]] std::size_t fetch_size(
      std::size_t requested,
      std::size_t remaining) const noexcept {
    const auto bounded = std::min(requested, remaining);
    if (bounded == 0 || capacity_bytes_ == 0)
      return bounded;
    return std::min(
        remaining,
        std::max(bounded, capacity_bytes_));
  }

  void store(
      std::uint64_t offset,
      std::vector<std::byte> bytes,
      std::size_t valid_bytes) noexcept {
    if (valid_bytes > bytes.size()) {
      clear();
      return;
    }
    bytes.resize(valid_bytes);
    offset_ = offset;
    data_ = std::move(bytes);
  }

  [[nodiscard]] bool copy(
      std::uint64_t offset,
      std::span<std::byte> destination,
      std::size_t wanted,
      std::size_t& out_read) const noexcept {
    out_read = 0;
    if (wanted == 0 || data_.empty() || offset < offset_)
      return false;

    const auto relative64 = offset - offset_;
    if (relative64 > static_cast<std::uint64_t>(data_.size()))
      return false;

    const auto relative = static_cast<std::size_t>(relative64);
    if (wanted > data_.size() - relative ||
        wanted > destination.size())
      return false;

    std::memcpy(
        destination.data(),
        data_.data() + relative,
        wanted);
    out_read = wanted;
    return true;
  }

private:
  std::size_t capacity_bytes_{};
  std::uint64_t offset_{};
  std::vector<std::byte> data_{};
};

} // namespace ps5rt::detail
