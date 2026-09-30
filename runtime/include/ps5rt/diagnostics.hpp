#pragma once

#include <cstddef>
#include <cstdint>

#include <ps5rt/result.hpp>

namespace ps5rt {

struct MemoryDiagnostics {
  std::size_t flexible_available{};
  std::size_t direct_available{};
  std::size_t executable_reserved{};
  std::size_t largest_known_free_range{};
};

struct RuntimeDiagnostics {
  MemoryDiagnostics memory{};
  std::uint32_t active_threads{};
  std::uint32_t active_controllers{};
};

Result collect_runtime_diagnostics(RuntimeDiagnostics& out) noexcept;

} // namespace ps5rt
