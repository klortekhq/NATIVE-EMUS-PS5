#pragma once

#include <corehost/libretro_abi.hpp>

namespace corehost {

struct Ps5RunnerConfig {
  const char* app_name{};
  const char* system_key{};
  const char* default_content{};
  const char* data_root{"/data/NATIVE-EMUS-PS5"};
};

// Runs one statically linked emulator core as a standalone PS5 application.
// Returns zero on a user-requested clean exit and a non-zero diagnostic code
// on initialization/content/runtime failure.
int run_linked_core_ps5(
    const Ps5RunnerConfig& config,
    lr::StaticApi api) noexcept;

} // namespace corehost
