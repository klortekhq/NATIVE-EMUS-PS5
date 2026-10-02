#include <ps5rt/diagnostics.hpp>

#include <ps5rt/input.hpp>
#include <ps5rt/memory.hpp>
#include <ps5rt/thread.hpp>

namespace ps5rt {

Result collect_runtime_diagnostics(
    RuntimeDiagnostics& out) noexcept {
  RuntimeDiagnostics candidate{};

  const auto memory =
      query_memory_diagnostics(
          candidate.memory);
  if (!memory) {
    out = {};
    return memory;
  }

  candidate.active_threads =
      managed_thread_count();
  candidate.active_controllers =
      connected_controller_count();

  out = candidate;
  return Result::success();
}

} // namespace ps5rt
