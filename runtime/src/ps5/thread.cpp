#include <ps5rt/thread.hpp>

#include <pthread.h>
#include <pthread_np.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>

extern "C" {
int scePthreadSetName(pthread_t thread, const char* name);
int scePthreadGetname(pthread_t thread, char* name);
int scePthreadSetaffinity(
    pthread_t thread,
    unsigned long long affinity);
}

namespace ps5rt {
namespace {

static_assert(std::is_trivially_copyable_v<pthread_t>);
static_assert(sizeof(pthread_t) <= sizeof(std::uintptr_t));

constexpr std::size_t kThreadNameBytes = max_thread_name_bytes;

std::atomic<std::uint32_t> g_managed_threads{0};

struct ThreadStart {
  ThreadEntry entry{};
  void* argument{};
  std::array<char, kThreadNameBytes> name{};
  std::int32_t affinity_hint{-1};
};

[[nodiscard]] bool copy_name(
    std::array<char, kThreadNameBytes>& out,
    const char* name) noexcept {
  if (name == nullptr || name[0] == '\0') {
    out[0] = '\0';
    return true;
  }

  const std::size_t length = std::strlen(name);
  if (length >= out.size()) {
    return false;
  }

  std::memcpy(
      out.data(),
      name,
      length + 1);
  return true;
}

[[nodiscard]] std::uintptr_t encode_thread(
    pthread_t thread) noexcept {
  std::uintptr_t value = 0;
  std::memcpy(&value, &thread, sizeof(thread));
  return value;
}

[[nodiscard]] pthread_t decode_thread(
    std::uintptr_t value) noexcept {
  pthread_t thread{};
  std::memcpy(&thread, &value, sizeof(thread));
  return thread;
}

void* thread_trampoline(void* raw) noexcept {
  ThreadStart* start =
      static_cast<ThreadStart*>(raw);

  const ThreadEntry entry = start->entry;
  void* const argument = start->argument;
  const std::int32_t affinity =
      start->affinity_hint;
  std::array<char, kThreadNameBytes> name =
      start->name;
  delete start;

  if (name[0] != '\0') {
    (void)set_current_thread_name(
        name.data());
  }
  if (affinity >= 0) {
    (void)set_current_thread_affinity(
        affinity);
  }

  entry(argument);
  g_managed_threads.fetch_sub(
      1,
      std::memory_order_acq_rel);
  return nullptr;
}

} // namespace

Result set_current_thread_name(
    const char* name) noexcept {
  if (name == nullptr || name[0] == '\0') {
    return {
        ErrorCode::invalid_argument,
        0,
        "thread name is empty"};
  }

  const int rc =
      scePthreadSetName(
          pthread_self(),
          name);
  if (rc != 0) {
    return {
        ErrorCode::system_error,
        rc,
        "scePthreadSetName failed"};
  }
  return Result::success();
}

Result query_current_thread_name(
    ThreadName& out) noexcept {
  out = {};

  const int rc =
      scePthreadGetname(
          pthread_self(),
          out.value.data());
  if (rc != 0) {
    out = {};
    return {
        ErrorCode::system_error,
        rc,
        "scePthreadGetname failed"};
  }

  if (out.value.back() != '\0') {
    out.value.back() = '\0';
    return {
        ErrorCode::system_error,
        0,
        "thread name was not terminated"};
  }

  return Result::success();
}

Result set_current_thread_affinity(
    std::int32_t cpu) noexcept {
  if (cpu < 0 || cpu >= 64) {
    return {
        ErrorCode::invalid_argument,
        0,
        "CPU index is outside 0..63"};
  }

  const auto mask =
      1ull << static_cast<unsigned>(cpu);
  const int rc =
      scePthreadSetaffinity(
          pthread_self(),
          mask);
  if (rc != 0) {
    return {
        ErrorCode::system_error,
        rc,
        "scePthreadSetaffinity failed"};
  }
  return Result::success();
}

Result query_current_thread_stack(
    std::size_t& out_bytes) noexcept {
  out_bytes = 0;

  pthread_attr_t attributes{};
  int rc = pthread_attr_init(&attributes);
  if (rc != 0) {
    return {
        ErrorCode::system_error,
        rc,
        "pthread_attr_init failed"};
  }

  rc = pthread_attr_get_np(
      pthread_self(),
      &attributes);
  if (rc == 0) {
    rc = pthread_attr_getstacksize(
        &attributes,
        &out_bytes);
  }

  const int destroy_rc =
      pthread_attr_destroy(&attributes);

  if (rc != 0) {
    out_bytes = 0;
    return {
        ErrorCode::system_error,
        rc,
        "current-thread stack query failed"};
  }
  if (destroy_rc != 0) {
    out_bytes = 0;
    return {
        ErrorCode::system_error,
        destroy_rc,
        "pthread_attr_destroy failed"};
  }
  if (out_bytes == 0) {
    return {
        ErrorCode::system_error,
        0,
        "current-thread stack size is zero"};
  }

  return Result::success();
}

Result query_current_thread_diagnostics(
    ThreadDiagnostics& out) noexcept {
  out = {};

  const auto name_result =
      query_current_thread_name(out.name);
  if (!name_result) {
    out = {};
    return name_result;
  }

  const auto stack_result =
      query_current_thread_stack(
          out.stack_bytes);
  if (!stack_result) {
    out = {};
    return stack_result;
  }

  out.managed_thread_count =
      managed_thread_count();
  return Result::success();
}

Result create_thread(
    ThreadHandle& out,
    const ThreadConfig& config,
    ThreadEntry entry,
    void* argument) noexcept {
  if (out.joinable) {
    return {
        ErrorCode::invalid_argument,
        0,
        "thread handle is already joinable"};
  }
  if (entry == nullptr) {
    return {
        ErrorCode::invalid_argument,
        0,
        "thread entry is null"};
  }
  if (config.priority != 0) {
    return {
        ErrorCode::unsupported,
        0,
        "explicit thread priority is not implemented"};
  }
  if (config.affinity_hint < -1 ||
      config.affinity_hint >= 64) {
    return {
        ErrorCode::invalid_argument,
        0,
        "thread affinity is outside -1..63"};
  }

  auto* start =
      new (std::nothrow) ThreadStart{};
  if (start == nullptr) {
    return {
        ErrorCode::out_of_memory,
        0,
        "thread start allocation failed"};
  }

  start->entry = entry;
  start->argument = argument;
  start->affinity_hint =
      config.affinity_hint;
  if (!copy_name(start->name, config.name)) {
    delete start;
    return {
        ErrorCode::invalid_argument,
        0,
        "thread name exceeds 63 bytes"};
  }

  pthread_attr_t attributes{};
  int rc = pthread_attr_init(&attributes);
  if (rc != 0) {
    delete start;
    return {
        ErrorCode::system_error,
        rc,
        "pthread_attr_init failed"};
  }

  if (config.stack_size != 0) {
    rc = pthread_attr_setstacksize(
        &attributes,
        config.stack_size);
    if (rc != 0) {
      (void)pthread_attr_destroy(
          &attributes);
      delete start;
      return {
          ErrorCode::invalid_argument,
          rc,
          "pthread_attr_setstacksize failed"};
    }
  }

  pthread_t thread{};
  g_managed_threads.fetch_add(
      1,
      std::memory_order_acq_rel);
  rc = pthread_create(
      &thread,
      &attributes,
      thread_trampoline,
      start);

  (void)pthread_attr_destroy(
      &attributes);

  if (rc != 0) {
    g_managed_threads.fetch_sub(
        1,
        std::memory_order_acq_rel);
    delete start;
    return {
        ErrorCode::system_error,
        rc,
        "pthread_create failed"};
  }

  out.native = encode_thread(thread);
  out.joinable = true;
  return Result::success();
}

Result join_thread(
    ThreadHandle& thread) noexcept {
  if (!thread.joinable) {
    return {
        ErrorCode::invalid_argument,
        0,
        "thread handle is not joinable"};
  }

  const pthread_t native =
      decode_thread(thread.native);
  const int rc =
      pthread_join(native, nullptr);
  if (rc != 0) {
    return {
        ErrorCode::system_error,
        rc,
        "pthread_join failed"};
  }

  thread = {};
  return Result::success();
}

std::uint32_t managed_thread_count() noexcept {
  return g_managed_threads.load(
      std::memory_order_acquire);
}

Result detach_thread(
    ThreadHandle& thread) noexcept {
  if (!thread.joinable) {
    return {
        ErrorCode::invalid_argument,
        0,
        "thread handle is not joinable"};
  }

  const pthread_t native =
      decode_thread(thread.native);
  const int rc =
      pthread_detach(native);
  if (rc != 0) {
    return {
        ErrorCode::system_error,
        rc,
        "pthread_detach failed"};
  }

  thread = {};
  return Result::success();
}

} // namespace ps5rt
