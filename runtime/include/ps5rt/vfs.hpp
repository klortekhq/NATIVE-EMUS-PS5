#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <ps5rt/result.hpp>

namespace ps5rt {

enum class StorageKind : std::uint8_t {
  app,
  data,
  internal,
  m2,
  usb,
  network,
};

struct StorageRoot {
  StorageKind kind{};
  std::string path{};
  bool writable{};
};

Result enumerate_storage(std::vector<StorageRoot>& out) noexcept;
Result ensure_directory(std::string_view path) noexcept;

// Normalize only host paths. Guest filesystem semantics stay inside the
// emulator core.
Result normalize_host_path(std::string_view input, std::string& out) noexcept;

} // namespace ps5rt
