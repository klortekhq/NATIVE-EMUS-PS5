#include <ps5rt/thread.hpp>

#include <cstddef>
#include <cstdio>

int main() {
  std::printf("ps5rt-thread-probe: start\n");
  std::fflush(stdout);

  const auto name =
      ps5rt::set_current_thread_name(
          "ps5rt-probe");
  if (!name) {
    std::printf(
        "ps5rt-thread-probe: name FAILED rc=%d\n",
        static_cast<int>(name.native_code));
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
        static_cast<int>(stack.native_code));
    std::fflush(stdout);
    return 2;
  }

  std::printf(
      "ps5rt-thread-probe: PASS stack=%zu recommended_jit=%zu\n",
      stack_bytes,
      ps5rt::recommended_stack_size(
          "jit",
          stack_bytes));
  std::fflush(stdout);
  return 0;
}
