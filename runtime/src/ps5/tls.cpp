#include <ps5rt/tls.hpp>

#include <atomic>
#include <cstdint>

namespace ps5rt {
namespace {

std::atomic<bool> g_tls_initialized{false};
std::atomic<TlsMode> g_tls_mode{TlsMode::automatic};

[[nodiscard]] Result unsupported_mode(
    TlsMode mode) noexcept {
  switch (mode) {
    case TlsMode::native:
      return {
          ErrorCode::unsupported,
          0,
          "native TLS is not validated on PS5"};
    case TlsMode::compatibility_shim:
      return {
          ErrorCode::unsupported,
          0,
          "TLS compatibility shim is not implemented"};
    case TlsMode::automatic:
    case TlsMode::compiler_rt_emutls:
      break;
  }
  return {
      ErrorCode::invalid_argument,
      0,
      "unknown TLS mode"};
}

}  // namespace

Result initialize_tls(
    TlsMode preferred,
    TlsInfo& out) noexcept {
  out = {};

  TlsMode selected = preferred;
  if (selected == TlsMode::automatic) {
    selected = TlsMode::compiler_rt_emutls;
  }

  if (selected != TlsMode::compiler_rt_emutls) {
    return unsupported_mode(selected);
  }

  const bool already_initialized =
      g_tls_initialized.load(
          std::memory_order_acquire);
  if (already_initialized) {
    const auto active =
        g_tls_mode.load(
            std::memory_order_acquire);
    if (active != selected) {
      return {
          ErrorCode::system_error,
          0,
          "TLS backend already initialized with another mode"};
    }
  } else {
    g_tls_mode.store(
        selected,
        std::memory_order_release);
    g_tls_initialized.store(
        true,
        std::memory_order_release);
  }

  out.mode = selected;
  // The public PS5 SDK's compiler-rt emutls implementation creates its
  // pthread key lazily from __emutls_get_address and owns per-thread cleanup
  // through that key's destructor. ps5rt therefore has no separate thread
  // registration object to allocate or destroy.
  out.thread_registration_required = false;
  return Result::success();
}

Result register_current_thread_tls() noexcept {
  if (!g_tls_initialized.load(
          std::memory_order_acquire)) {
    return {
        ErrorCode::invalid_argument,
        0,
        "TLS is not initialized"};
  }

  if (g_tls_mode.load(
          std::memory_order_acquire) !=
      TlsMode::compiler_rt_emutls) {
    return {
        ErrorCode::unsupported,
        0,
        "active TLS backend requires explicit registration"};
  }

  return Result::success();
}

void unregister_current_thread_tls() noexcept {
  // compiler-rt emutls stores the per-thread address array in a pthread key.
  // Its libc-owned key destructor releases that array when the thread exits.
}

void shutdown_tls() noexcept {
  // This is ps5rt policy teardown only. The public SDK does not expose a
  // non-Bionic API for deleting compiler-rt's process-global emutls key, and
  // forcibly deleting it would also invalidate TLS owned by other libraries.
  g_tls_initialized.store(
      false,
      std::memory_order_release);
  g_tls_mode.store(
      TlsMode::automatic,
      std::memory_order_release);
}

}  // namespace ps5rt
