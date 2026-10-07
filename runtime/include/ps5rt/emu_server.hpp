#pragma once

#include <cstddef>
#include <string_view>

#include <ps5rt/result.hpp>

namespace ps5rt {

struct EmuServerConfig {
  std::string_view bearer_token{};
  std::string_view user_agent{"NATIVE-EMUS-PS5/1"};
  // 0 keeps exact-range behavior. Non-zero values request at least this many
  // bytes on a cache miss so nearby sequential reads can be served locally.
  // Hardware benchmarks should choose the final policy rather than hardcoding
  // an emulator-independent guess.
  std::size_t read_ahead_bytes{};
  // 0 preserves fixed read_ahead_bytes behavior. A larger value enables
  // deterministic doubling after consecutive logical reads until this cap.
  // Random seeks reset the window to read_ahead_bytes.
  std::size_t max_read_ahead_bytes{};
};

// Registers the read-only emus:// backend.
//
// URI shape:
//   emus://host:port/<catalog-file-id>/<virtual-relative-path>
//
// It is translated to the server file endpoint while the optional suffix is
// forwarded as an encoded virtual path anchored at the catalog entry's
// directory. This lets descriptor formats (CUE/CCD/TOC/M3U) open relative
// sidecars without exposing the server's physical filesystem.
//
// The token is kept in process-local configuration and is never embedded in
// the URI. Call shutdown_emu_server_backend() before process teardown.
Result initialize_emu_server_backend(const EmuServerConfig& config = {}) noexcept;
void shutdown_emu_server_backend() noexcept;

} // namespace ps5rt
