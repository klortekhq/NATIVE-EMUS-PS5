#include <ps5rt/thread.hpp>

#include <atomic>
#include <cstdint>
#include <cstdio>

namespace {

thread_local std::uint64_t g_tls_value =
    UINT64_C(0x1122334455667788);

struct WorkerState {
  std::uint64_t expected{};
  std::atomic<std::uint32_t>* ready{};
  std::atomic<bool>* release{};
  std::atomic<bool> ok{false};
};

void tls_worker(void* raw) noexcept {
  auto& state =
      *static_cast<WorkerState*>(raw);

  g_tls_value = state.expected;
  state.ready->fetch_add(
      1,
      std::memory_order_acq_rel);

  while (!state.release->load(
      std::memory_order_acquire)) {
  }

  state.ok.store(
      g_tls_value == state.expected,
      std::memory_order_release);
}

} // namespace

int main() {
  std::printf("ps5rt-emutls-probe: start\n");
  std::fflush(stdout);

  constexpr std::uint64_t main_value =
      UINT64_C(0xa5a5a5a55a5a5a5a);
  g_tls_value = main_value;

  std::atomic<std::uint32_t> ready{0};
  std::atomic<bool> release{false};

  WorkerState first{
      UINT64_C(0x0102030405060708),
      &ready,
      &release};
  WorkerState second{
      UINT64_C(0x8877665544332211),
      &ready,
      &release};

  ps5rt::ThreadHandle first_thread{};
  ps5rt::ThreadHandle second_thread{};

  ps5rt::ThreadConfig first_config{};
  first_config.name = "ps5rt-tls-a";
  first_config.stack_size =
      2u * 1024u * 1024u;

  ps5rt::ThreadConfig second_config{};
  second_config.name = "ps5rt-tls-b";
  second_config.stack_size =
      2u * 1024u * 1024u;

  const auto first_created =
      ps5rt::create_thread(
          first_thread,
          first_config,
          tls_worker,
          &first);
  if (!first_created) {
    std::printf(
        "ps5rt-emutls-probe: first create FAILED rc=%d\n",
        static_cast<int>(
            first_created.native_code));
    std::fflush(stdout);
    return 1;
  }

  const auto second_created =
      ps5rt::create_thread(
          second_thread,
          second_config,
          tls_worker,
          &second);
  if (!second_created) {
    release.store(
        true,
        std::memory_order_release);
    (void)ps5rt::join_thread(
        first_thread);
    std::printf(
        "ps5rt-emutls-probe: second create FAILED rc=%d\n",
        static_cast<int>(
            second_created.native_code));
    std::fflush(stdout);
    return 2;
  }

  while (ready.load(
      std::memory_order_acquire) != 2u) {
  }

  const bool main_unchanged_before_release =
      g_tls_value == main_value;

  release.store(
      true,
      std::memory_order_release);

  const auto first_joined =
      ps5rt::join_thread(first_thread);
  const auto second_joined =
      ps5rt::join_thread(second_thread);

  const bool isolated =
      first_joined &&
      second_joined &&
      main_unchanged_before_release &&
      g_tls_value == main_value &&
      first.ok.load(
          std::memory_order_acquire) &&
      second.ok.load(
          std::memory_order_acquire);

  if (!isolated) {
    std::printf(
        "ps5rt-emutls-probe: FAILED main=%d first=%d second=%d\n",
        g_tls_value == main_value ? 1 : 0,
        first.ok.load(
            std::memory_order_acquire) ? 1 : 0,
        second.ok.load(
            std::memory_order_acquire) ? 1 : 0);
    std::fflush(stdout);
    return 3;
  }

  std::printf(
      "ps5rt-emutls-probe: PASS isolated=3\n");
  std::fflush(stdout);
  return 0;
}
