#include <ps5rt/thread.hpp>

#include <atomic>
#include <cstddef>
#include <cstdio>

namespace {

struct ProbeState {
  std::atomic<bool> ran{false};
  std::atomic<bool> stack_ok{false};
};

void child_entry(void* raw) noexcept {
  auto& state =
      *static_cast<ProbeState*>(raw);

  std::size_t stack_bytes = 0;
  const auto stack =
      ps5rt::query_current_thread_stack(
          stack_bytes);
  if (stack &&
      stack_bytes >=
          3u * 1024u * 1024u) {
    state.stack_ok.store(
        true,
        std::memory_order_release);
  }

  state.ran.store(
      true,
      std::memory_order_release);
}

} // namespace

int main() {
  std::printf("ps5rt-thread-probe: start\n");
  std::fflush(stdout);

  const auto name =
      ps5rt::set_current_thread_name(
          "ps5rt-probe");
  if (!name) {
    std::printf(
        "ps5rt-thread-probe: name FAILED rc=%d\n",
        static_cast<int>(
            name.native_code));
    std::fflush(stdout);
    return 1;
  }

  std::size_t stack_bytes = 0;
  const auto stack =
      ps5rt::query_current_thread_stack(
          stack_bytes);
  if (!stack) {
    std::printf(
        "ps5rt-thread-probe: stack FAILED rc=%d\n",
        static_cast<int>(
            stack.native_code));
    std::fflush(stdout);
    return 2;
  }

  ProbeState state{};
  ps5rt::ThreadHandle child{};
  ps5rt::ThreadConfig config{};
  config.name = "ps5rt-child";
  config.stack_size =
      3u * 1024u * 1024u;

  const auto created =
      ps5rt::create_thread(
          child,
          config,
          child_entry,
          &state);
  if (!created) {
    std::printf(
        "ps5rt-thread-probe: create FAILED rc=%d\n",
        static_cast<int>(
            created.native_code));
    std::fflush(stdout);
    return 3;
  }

  const auto joined =
      ps5rt::join_thread(child);
  if (!joined ||
      !state.ran.load(
          std::memory_order_acquire) ||
      !state.stack_ok.load(
          std::memory_order_acquire)) {
    std::printf(
        "ps5rt-thread-probe: child FAILED join=%d ran=%d stack_ok=%d\n",
        static_cast<int>(
            joined.native_code),
        state.ran.load(
            std::memory_order_acquire) ? 1 : 0,
        state.stack_ok.load(
            std::memory_order_acquire) ? 1 : 0);
    std::fflush(stdout);
    return 4;
  }

  std::printf(
      "ps5rt-thread-probe: PASS stack=%zu recommended_jit=%zu child_stack>=3145728\n",
      stack_bytes,
      ps5rt::recommended_stack_size(
          "jit",
          stack_bytes));
  std::fflush(stdout);
  return 0;
}
