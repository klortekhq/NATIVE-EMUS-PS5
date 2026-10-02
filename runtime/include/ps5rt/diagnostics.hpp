#pragma once

#include <cstddef>
#include <cstdint>

#include <ps5rt/result.hpp>

namespace ps5rt {

struct MemoryDiagnostics {
  // Existing coarse counters retained for source compatibility.
  std::size_t flexible_available{};
  std::size_t direct_available{};
  std::size_t executable_reserved{};
  std::size_t largest_known_free_range{};

  // Detailed PS5 allocator pressure/ownership counters.
  std::size_t direct_aperture_bytes{};
  std::size_t pool_capacity_bytes{};
  std::size_t pool_committed_bytes{};
  std::size_t pool_available_bytes{};

  std::size_t tracked_direct_mapping_count{};
  std::size_t tracked_direct_bytes{};
  std::size_t tracked_pool_mapping_count{};
  std::size_t tracked_pool_bytes{};
  std::size_t tracked_executable_mapping_count{};
  std::size_t tracked_executable_bytes{};
  std::size_t tracked_dual_jit_region_count{};
  std::size_t tracked_dual_jit_bytes{};
  std::size_t tracked_sparse_arena_count{};
  std::size_t tracked_sparse_reserved_bytes{};
  std::size_t tracked_sparse_committed_bytes{};
  std::size_t tracked_sparse_executable_bytes{};
};

struct RuntimeDiagnostics {
  MemoryDiagnostics memory{};
  std::uint32_t active_threads{};
  std::uint32_t active_controllers{};
};

Result collect_runtime_diagnostics(RuntimeDiagnostics& out) noexcept;

} // namespace ps5rt
