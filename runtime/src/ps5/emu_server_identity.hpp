#pragma once

#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>

namespace ps5rt::detail {

enum class EmuServerIdentityResult {
  initialized,
  unchanged,
  changed,
  invalid_contract,
};

[[nodiscard]] inline bool
ascii_equal_ci(std::string_view lhs, std::string_view rhs) noexcept {
  if (lhs.size() != rhs.size())
    return false;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    const auto a = static_cast<unsigned char>(lhs[i]);
    const auto b = static_cast<unsigned char>(rhs[i]);
    if (std::tolower(a) != std::tolower(b))
      return false;
  }
  return true;
}

class EmuServerObjectIdentity {
public:
  [[nodiscard]] EmuServerIdentityResult observe(
      std::uint64_t size,
      std::string_view etag,
      std::string_view accept_ranges) {
    if (etag.empty() || !ascii_equal_ci(accept_ranges, "bytes"))
      return EmuServerIdentityResult::invalid_contract;

    if (!initialized_) {
      initialized_ = true;
      size_ = size;
      etag_.assign(etag);
      return EmuServerIdentityResult::initialized;
    }

    if (size_ != size || etag_ != etag)
      return EmuServerIdentityResult::changed;

    return EmuServerIdentityResult::unchanged;
  }

  [[nodiscard]] bool initialized() const noexcept { return initialized_; }
  [[nodiscard]] std::uint64_t size() const noexcept { return size_; }
  [[nodiscard]] std::string_view etag() const noexcept { return etag_; }

private:
  bool initialized_{};
  std::uint64_t size_{};
  std::string etag_{};
};

} // namespace ps5rt::detail
