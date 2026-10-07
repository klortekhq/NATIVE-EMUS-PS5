#include <ps5rt/diagnostics.hpp>
#include <ps5rt/input.hpp>
#include <ps5rt/memory.hpp>
#include <ps5rt/thread.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace ps5rt {

Result query_memory_diagnostics(
    MemoryDiagnostics& out) noexcept {
  out = {};
  out.flexible_available =
      512u * 1024u * 1024u;
  out.direct_available =
      1536u * 1024u * 1024u;
  out.tracked_executable_bytes =
      64u * 1024u * 1024u;
  return Result::success();
}

std::uint32_t managed_thread_count() noexcept {
  return 7;
}

std::uint32_t
connected_controller_count() noexcept {
  return 3;
}

} // namespace ps5rt

int main() {
  ps5rt::RuntimeDiagnostics diagnostics{};
  assert(ps5rt::collect_runtime_diagnostics(
      diagnostics));

  assert(
      diagnostics.memory.flexible_available ==
      512u * 1024u * 1024u);
  assert(
      diagnostics.memory.direct_available ==
      1536u * 1024u * 1024u);
  assert(
      diagnostics.memory.tracked_executable_bytes ==
      64u * 1024u * 1024u);
  assert(diagnostics.active_threads == 7);
  assert(diagnostics.active_controllers == 3);

  return 0;
}
