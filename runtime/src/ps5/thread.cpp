#include <ps5rt/thread.hpp>

#include <pthread.h>
#include <pthread_np.h>

#include <cstddef>
#include <cstdint>

extern "C" {
int scePthreadSetName(pthread_t thread, const char* name);
int scePthreadSetaffinity(
    pthread_t thread,
    unsigned long long affinity);
}

namespace ps5rt {

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

} // namespace ps5rt
