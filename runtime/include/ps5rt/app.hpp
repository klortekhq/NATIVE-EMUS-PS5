#pragma once

#include <cstdint>
#include <string_view>

#include <ps5rt/result.hpp>

namespace ps5rt {

struct AppConfig {
  std::string_view app_name{};
  std::string_view data_directory{};
  bool initialize_user_service{true};
  bool initialize_network{false};
  bool enable_crash_log{true};
};

struct AppInfo {
  std::uint32_t active_user_id{};
  bool network_available{};
};

Result initialize_app(const AppConfig& config, AppInfo& out) noexcept;
void shutdown_app() noexcept;

} // namespace ps5rt
