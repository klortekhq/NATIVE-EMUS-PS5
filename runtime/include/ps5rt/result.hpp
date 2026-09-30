#pragma once

#include <cstdint>
#include <string_view>

namespace ps5rt {

enum class ErrorCode : std::int32_t {
  ok = 0,
  invalid_argument,
  unsupported,
  out_of_memory,
  permission_denied,
  io_error,
  system_error,
};

struct Result {
  ErrorCode code{ErrorCode::ok};
  std::int32_t native_code{0};
  std::string_view message{};

  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return code == ErrorCode::ok;
  }

  [[nodiscard]] static constexpr Result success() noexcept {
    return {};
  }
};

} // namespace ps5rt
