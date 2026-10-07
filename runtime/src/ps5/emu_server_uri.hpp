#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace ps5rt::detail {

inline bool emus_query_unreserved(unsigned char ch) noexcept {
  return
      (ch >= 'a' && ch <= 'z') ||
      (ch >= 'A' && ch <= 'Z') ||
      (ch >= '0' && ch <= '9') ||
      ch == '-' || ch == '.' || ch == '_' || ch == '~';
}

inline void append_emus_query_encoded(
    std::string_view value,
    std::string& out) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  for (const unsigned char ch : value) {
    if (emus_query_unreserved(ch)) {
      out.push_back(static_cast<char>(ch));
      continue;
    }
    out.push_back('%');
    out.push_back(kHex[(ch >> 4u) & 0x0fu]);
    out.push_back(kHex[ch & 0x0fu]);
  }
}

inline bool emus_to_http(std::string_view uri, std::string& out) {
  constexpr std::string_view prefix = "emus://";
  if (!uri.starts_with(prefix))
    return false;

  const auto rest = uri.substr(prefix.size());
  const auto slash = rest.find('/');
  if (slash == std::string_view::npos || slash == 0 || slash + 1 >= rest.size())
    return false;

  const auto authority = rest.substr(0, slash);
  const auto resource = rest.substr(slash + 1);
  const auto metadata_slash = resource.find('/');
  const auto id = resource.substr(0, metadata_slash);

  if (id.size() != 64)
    return false;
  for (const char ch : id) {
    const bool hex =
        (ch >= '0' && ch <= '9') ||
        (ch >= 'a' && ch <= 'f') ||
        (ch >= 'A' && ch <= 'F');
    if (!hex)
      return false;
  }

  if (authority.find_first_of(" \t\r\n?#") != std::string_view::npos)
    return false;

  std::string_view metadata{};
  if (metadata_slash != std::string_view::npos) {
    metadata = resource.substr(metadata_slash + 1);
    if (metadata.empty() || metadata.size() > 4096 ||
        metadata.find('\0') != std::string_view::npos ||
        metadata.find_first_of("\r\n") != std::string_view::npos)
      return false;
  }

  out = "http://";
  out.append(authority);
  out.append("/api/v1/files/");
  out.append(id);

  if (!metadata.empty()) {
    out.append("?path=");
    append_emus_query_encoded(metadata, out);
  }
  return true;
}

} // namespace ps5rt::detail
