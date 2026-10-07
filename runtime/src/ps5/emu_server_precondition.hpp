#pragma once

#include <string_view>

namespace ps5rt::detail {

inline constexpr std::string_view kEmuServerEntityPreconditionHeader =
    "If-Match";

enum class EmuServerRangeStatus {
  partial_content,
  stale_object,
  unexpected,
};

[[nodiscard]] constexpr EmuServerRangeStatus
classify_emu_server_range_status(int status) noexcept {
  switch (status) {
  case 206:
    return EmuServerRangeStatus::partial_content;
  case 412:
    return EmuServerRangeStatus::stale_object;
  default:
    return EmuServerRangeStatus::unexpected;
  }
}

} // namespace ps5rt::detail
