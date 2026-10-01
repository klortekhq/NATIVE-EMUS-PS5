#include "config.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string_view>

namespace native_emus::ps1 {
namespace {

std::string trim(std::string value) {
  const auto not_space = [](unsigned char c) { return !std::isspace(c); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  return value;
}

bool one_of(std::string_view value, std::initializer_list<std::string_view> allowed) {
  return std::find(allowed.begin(), allowed.end(), value) != allowed.end();
}

bool assign(
    PortConfig& config,
    std::string_view key,
    const std::string& value,
    std::string& error) {
  if (key == "region") {
    if (!one_of(value, {"auto", "ntsc-j", "ntsc-u", "pal"})) {
      error = "invalid PS1 region: " + value;
      return false;
    }
    config.region = value;
    return true;
  }

  if (key == "bios") {
    if (!one_of(value, {"auto", "openbios", "psxonpsp", "ps1_rom"})) {
      error = "invalid PS1 BIOS override: " + value;
      return false;
    }
    config.bios_override = value == "auto" ? "disabled" : value;
    return true;
  }

  if (key == "skip_bios") {
    if (!one_of(value, {"0", "1", "false", "true", "disabled", "enabled"})) {
      error = "invalid PS1 skip_bios value: " + value;
      return false;
    }
    config.skip_bios =
        (value == "1" || value == "true" || value == "enabled")
            ? "enabled"
            : "disabled";
    return true;
  }

  if (key == "internal_resolution") {
    if (!one_of(value, {"1x(native)", "2x", "4x", "8x", "16x"})) {
      error = "invalid PS1 internal_resolution: " + value;
      return false;
    }
    config.internal_resolution = value;
    return true;
  }

  if (key == "server_token") {
    if (value.size() > 1024 ||
        value.find('\r') != std::string::npos ||
        value.find('\n') != std::string::npos) {
      error = "invalid PS1 server_token";
      return false;
    }
    config.server_token = value;
    return true;
  }

  // Forward-compatible: ignore settings intended for newer builds.
  return true;
}

} // namespace

bool load_port_config(
    const std::filesystem::path& path,
    PortConfig& out,
    std::string& error) {
  out = {};
  error.clear();

  std::error_code ec;
  if (!std::filesystem::exists(path, ec))
    return !ec;
  if (ec) {
    error = "cannot stat PS1 config";
    return false;
  }

  std::ifstream input(path);
  if (!input) {
    error = "cannot open PS1 config";
    return false;
  }

  std::string line;
  unsigned line_number = 0;
  while (std::getline(input, line)) {
    ++line_number;
    line = trim(std::move(line));
    if (line.empty() || line.front() == '#' || line.front() == ';')
      continue;

    const auto equals = line.find('=');
    if (equals == std::string::npos) {
      error = "invalid PS1 config line " + std::to_string(line_number);
      return false;
    }

    const std::string key = trim(line.substr(0, equals));
    const std::string value = trim(line.substr(equals + 1));
    if (key.empty()) {
      error = "empty PS1 config key on line " + std::to_string(line_number);
      return false;
    }
    if (!assign(out, key, value, error)) {
      error += " on line " + std::to_string(line_number);
      return false;
    }
  }

  return true;
}

void apply_port_config(
    corehost::StaticCore& core,
    const PortConfig& config) {
  // These two are architecture, not preferences.
  core.set_option("beetle_psx_hw_renderer", "hardware_vk");
  core.set_option("beetle_psx_hw_cpu_dynarec", "execute");

  core.set_option("beetle_psx_hw_region", config.region);
  core.set_option("beetle_psx_hw_skip_bios", config.skip_bios);
  core.set_option("beetle_psx_hw_override_bios", config.bios_override);
  core.set_option(
      "beetle_psx_hw_internal_resolution",
      config.internal_resolution);
}

} // namespace native_emus::ps1
