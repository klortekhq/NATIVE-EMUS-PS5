#pragma once

#include <string_view>

#include <ps5rt/result.hpp>

namespace ps5rt {

struct EmuServerConfig {
  std::string_view bearer_token{};
  std::string_view user_agent{"NATIVE-EMUS-PS5/1"};
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
