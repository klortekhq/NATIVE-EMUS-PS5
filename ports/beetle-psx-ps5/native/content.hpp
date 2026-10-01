#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <corehost/static_core.hpp>

namespace native_emus::ps1 {

struct ContentLayout {
  std::filesystem::path data_root{"/data/NATIVE-EMUS-PS5/ps1"};
  std::filesystem::path system_dir{};
  std::filesystem::path save_dir{};

  ContentLayout();
};

struct PreparedContent {
  std::string core_path;
  std::filesystem::path save_ram_path;
  std::vector<std::filesystem::path> playlist_entries{};
  bool local_file{};
};

[[nodiscard]] bool is_supported_ps1_content(std::string_view path) noexcept;

bool prepare_content(
    std::string_view uri_or_path,
    const ContentLayout& layout,
    PreparedContent& out,
    std::string& error);

class SaveRamStore final {
public:
  explicit SaveRamStore(std::filesystem::path path);

  bool load(corehost::StaticCore& core, std::string& error) const;
  bool save(corehost::StaticCore& core, std::string& error) const;

  [[nodiscard]] const std::filesystem::path& path() const noexcept {
    return path_;
  }

private:
  std::filesystem::path path_;
};

} // namespace native_emus::ps1
