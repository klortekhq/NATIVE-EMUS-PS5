#pragma once

#include <cstddef>
#include <cstdint>

#include <ps5rt/diagnostics.hpp>
#include <ps5rt/result.hpp>

namespace ps5rt {

enum class MemoryKind : std::uint8_t {
  flexible,
  pooled,
  direct,
  executable,
};

enum class Protection : std::uint8_t {
  none = 0,
  read = 1u << 0,
  write = 1u << 1,
  execute = 1u << 2,
};

[[nodiscard]] constexpr Protection operator|(Protection a, Protection b) noexcept {
  return static_cast<Protection>(
      static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

struct Mapping {
  void* address{};
  std::size_t size{};
  MemoryKind kind{MemoryKind::flexible};
  Protection protection{Protection::none};

  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return address != nullptr && size != 0;
  }
};

struct MemoryRequest {
  std::size_t size{};
  std::size_t alignment{};
  Protection protection{Protection::read | Protection::write};
  void* preferred_address{};
  bool fixed_address{false};
  const char* debug_name{};
};

Result allocate_memory(MemoryKind kind, const MemoryRequest& request, Mapping& out) noexcept;
Result release_memory(Mapping& mapping) noexcept;
Result query_available_memory(MemoryKind kind, std::size_t& out_bytes) noexcept;
Result query_memory_diagnostics(MemoryDiagnostics& out) noexcept;

} // namespace ps5rt
