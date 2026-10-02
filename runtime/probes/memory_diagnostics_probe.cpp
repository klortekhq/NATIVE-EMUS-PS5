#include <ps5rt/memory.hpp>

#include <cstdio>

int main() {
  ps5rt::MemoryDiagnostics diagnostics{};
  const auto result =
      ps5rt::query_memory_diagnostics(diagnostics);
  if (!result) {
    std::printf(
        "ps5rt-memory-diagnostics-probe: FAILED code=%d native=%d\n",
        static_cast<int>(result.code),
        result.native_code);
    return 1;
  }

  std::printf(
      "ps5rt-memory-diagnostics-probe: "
      "flexible=%zu direct_aperture=%zu direct_largest=%zu "
      "pool_capacity=%zu pool_committed=%zu pool_available=%zu "
      "direct_maps=%zu direct_bytes=%zu "
      "pool_maps=%zu pool_bytes=%zu "
      "exec_maps=%zu exec_bytes=%zu "
      "dual_jit=%zu dual_jit_bytes=%zu "
      "sparse_arenas=%zu sparse_reserved=%zu sparse_committed=%zu\n",
      diagnostics.flexible_available_bytes,
      diagnostics.direct_aperture_bytes,
      diagnostics.direct_largest_available_block_bytes,
      diagnostics.pool_capacity_bytes,
      diagnostics.pool_committed_bytes,
      diagnostics.pool_available_bytes,
      diagnostics.tracked_direct_mapping_count,
      diagnostics.tracked_direct_bytes,
      diagnostics.tracked_pool_mapping_count,
      diagnostics.tracked_pool_bytes,
      diagnostics.tracked_executable_mapping_count,
      diagnostics.tracked_executable_bytes,
      diagnostics.tracked_dual_jit_region_count,
      diagnostics.tracked_dual_jit_bytes,
      diagnostics.tracked_sparse_arena_count,
      diagnostics.tracked_sparse_reserved_bytes,
      diagnostics.tracked_sparse_committed_bytes);

  if (diagnostics.direct_aperture_bytes == 0 ||
      diagnostics.direct_largest_available_block_bytes >
          diagnostics.direct_aperture_bytes ||
      diagnostics.pool_committed_bytes >
          diagnostics.pool_capacity_bytes) {
    std::printf(
        "ps5rt-memory-diagnostics-probe: invariant FAILED\n");
    return 2;
  }

  std::printf("ps5rt-memory-diagnostics-probe: PASS\n");
  return 0;
}
