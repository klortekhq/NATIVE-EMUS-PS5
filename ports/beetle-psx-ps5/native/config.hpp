#pragma once

#include <filesystem>
#include <string>

#include <corehost/static_core.hpp>

namespace native_emus::ps1 {

struct PortConfig {
  std::string region{"auto"};
  std::string bios_override{"disabled"};
  std::string skip_bios{"disabled"};
  std::string internal_resolution{"1x(native)"};
  std::string server_token{};
};

// Missing files are not an error: defaults are intentionally valid.
bool load_port_config(
    const std::filesystem::path& path,
    PortConfig& out,
    std::string& error);

void apply_port_config(
    corehost::StaticCore& core,
    const PortConfig& config);

} // namespace native_emus::ps1
