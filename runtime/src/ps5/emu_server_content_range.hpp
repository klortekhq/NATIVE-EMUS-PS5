#pragma once

#include <cctype>
#include <cstdint>
#include <limits>
#include <string_view>

namespace ps5rt::detail {

struct EmuServerContentRange {
  std::uint64_t first{};
  std::uint64_t last{};
  std::uint64_t total{};
};

[[nodiscard]] inline bool
parse_decimal_u64(
    std::string_view text,
    std::uint64_t& output) noexcept {
  output = 0;
  if (text.empty())
    return false;

  for (const char ch : text) {
    if (ch < '0' || ch > '9')
      return false;
    const auto digit = static_cast<std::uint64_t>(ch - '0');
    if (output >
        (std::numeric_limits<std::uint64_t>::max() - digit) / 10u)
      return false;
    output = output * 10u + digit;
  }
  return true;
}

[[nodiscard]] inline bool
ascii_starts_with_ci(
    std::string_view value,
    std::string_view prefix) noexcept {
  if (value.size() < prefix.size())
    return false;
  for (std::size_t i = 0; i < prefix.size(); ++i) {
    const auto lhs = static_cast<unsigned char>(value[i]);
    const auto rhs = static_cast<unsigned char>(prefix[i]);
    if (std::tolower(lhs) != std::tolower(rhs))
      return false;
  }
  return true;
}

[[nodiscard]] inline bool
parse_emu_server_content_range(
    std::string_view value,
    EmuServerContentRange& output) noexcept {
  output = {};

  while (!value.empty() &&
         (value.front() == ' ' || value.front() == '\t'))
    value.remove_prefix(1);
  while (!value.empty() &&
         (value.back() == ' ' || value.back() == '\t'))
    value.remove_suffix(1);

  constexpr std::string_view prefix = "bytes ";
  if (!ascii_starts_with_ci(value, prefix))
    return false;
  value.remove_prefix(prefix.size());

  const auto dash = value.find('-');
  const auto slash = value.find('/');
  if (dash == std::string_view::npos ||
      slash == std::string_view::npos ||
      dash == 0 ||
      slash <= dash + 1 ||
      slash + 1 >= value.size() ||
      value.find('-', dash + 1) != std::string_view::npos ||
      value.find('/', slash + 1) != std::string_view::npos)
    return false;

  // A 206 response for the emulation reader must identify one concrete range
  // and a concrete complete length. Wildcard forms are not stable enough for
  // a pinned emulation session.
  if (value.find('*') != std::string_view::npos)
    return false;

  EmuServerContentRange parsed{};
  if (!parse_decimal_u64(value.substr(0, dash), parsed.first) ||
      !parse_decimal_u64(
          value.substr(dash + 1, slash - dash - 1),
          parsed.last) ||
      !parse_decimal_u64(value.substr(slash + 1), parsed.total))
    return false;

  if (parsed.total == 0 ||
      parsed.first > parsed.last ||
      parsed.last >= parsed.total)
    return false;

  output = parsed;
  return true;
}

[[nodiscard]] inline bool
matches_emu_server_content_range(
    std::string_view value,
    std::uint64_t requested_first,
    std::uint64_t requested_last,
    std::uint64_t expected_total) noexcept {
  EmuServerContentRange parsed{};
  return parse_emu_server_content_range(value, parsed) &&
         parsed.first == requested_first &&
         parsed.last == requested_last &&
         parsed.total == expected_total;
}

} // namespace ps5rt::detail
