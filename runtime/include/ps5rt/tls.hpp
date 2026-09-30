#pragma once

#include <cstdint>

#include <ps5rt/result.hpp>

namespace ps5rt {

enum class TlsMode : std::uint8_t {
  automatic,
  native,
  compiler_rt_emutls,
  compatibility_shim,
};

struct TlsInfo {
  TlsMode mode{TlsMode::automatic};
  bool thread_registration_required{false};
};

Result initialize_tls(TlsMode preferred, TlsInfo& out) noexcept;
Result register_current_thread_tls() noexcept;
void unregister_current_thread_tls() noexcept;
void shutdown_tls() noexcept;

} // namespace ps5rt
