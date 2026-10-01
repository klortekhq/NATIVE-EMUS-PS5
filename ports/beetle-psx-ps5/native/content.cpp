#include "content.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <system_error>
#include <utility>
#include <vector>

namespace native_emus::ps1 {
namespace {

std::string lowercase(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

std::string strip_file_uri(std::string_view input) {
  constexpr std::string_view file_prefix = "file://";
  if (input.substr(0, file_prefix.size()) == file_prefix)
    return std::string(input.substr(file_prefix.size()));
  return std::string(input);
}

std::string save_stem(std::filesystem::path path) {
  auto stem = path.stem().string();
  if (stem.empty())
    stem = "ps1-content";
  return stem;
}

bool ensure_directory(const std::filesystem::path& path, std::string& error) {
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
  if (ec) {
    error = "cannot create directory: " + path.string();
    return false;
  }
  return true;
}

} // namespace

ContentLayout::ContentLayout() {
  system_dir = data_root / "system";
  save_dir = data_root / "saves";
}

bool is_supported_ps1_content(std::string_view path) noexcept {
  if (path.empty())
    return false;

  std::filesystem::path parsed(strip_file_uri(path));
  const auto ext = lowercase(parsed.extension().string());
  static constexpr std::array<std::string_view, 8> supported{
      ".cue", ".chd", ".pbp", ".iso", ".ccd", ".toc", ".m3u", ".exe"};

  return std::find(supported.begin(), supported.end(), ext) != supported.end();
}

bool prepare_content(
    std::string_view uri_or_path,
    const ContentLayout& layout,
    PreparedContent& out,
    std::string& error) {
  out = {};
  error.clear();

  if (uri_or_path.empty()) {
    error = "empty PS1 content path";
    return false;
  }

  const auto scheme = uri_or_path.find("://");
  if (scheme != std::string_view::npos &&
      uri_or_path.substr(0, scheme) != "file") {
    error = "non-local URI requires the ps5rt VFS bridge";
    return false;
  }

  const std::string local = strip_file_uri(uri_or_path);
  if (!is_supported_ps1_content(local)) {
    error = "unsupported PS1 content extension";
    return false;
  }

  std::filesystem::path content(local);
  std::error_code ec;
  if (!std::filesystem::exists(content, ec) || ec) {
    error = "PS1 content does not exist";
    return false;
  }
  if (!std::filesystem::is_regular_file(content, ec) || ec) {
    error = "PS1 content is not a regular file";
    return false;
  }

  if (!ensure_directory(layout.system_dir, error) ||
      !ensure_directory(layout.save_dir, error))
    return false;

  out.core_path = content.string();
  out.save_ram_path = layout.save_dir / (save_stem(content) + ".srm");
  out.local_file = true;
  return true;
}

SaveRamStore::SaveRamStore(std::filesystem::path path)
    : path_(std::move(path)) {}

bool SaveRamStore::load(corehost::StaticCore& core, std::string& error) const {
  error.clear();
  void* memory = core.memory_data(corehost::lr::memory_save_ram);
  const std::size_t size = core.memory_size(corehost::lr::memory_save_ram);
  if (!memory || !size)
    return true;

  std::error_code ec;
  if (!std::filesystem::exists(path_, ec))
    return !ec;
  if (ec) {
    error = "cannot stat save RAM";
    return false;
  }

  std::ifstream input(path_, std::ios::binary);
  if (!input) {
    error = "cannot open save RAM";
    return false;
  }

  input.seekg(0, std::ios::end);
  const auto file_size = input.tellg();
  if (file_size < 0 || static_cast<std::uint64_t>(file_size) != size) {
    error = "save RAM size does not match core";
    return false;
  }
  input.seekg(0, std::ios::beg);
  input.read(static_cast<char*>(memory), static_cast<std::streamsize>(size));
  if (!input) {
    error = "cannot read save RAM";
    return false;
  }
  return true;
}

bool SaveRamStore::save(corehost::StaticCore& core, std::string& error) const {
  error.clear();
  const void* memory = core.memory_data(corehost::lr::memory_save_ram);
  const std::size_t size = core.memory_size(corehost::lr::memory_save_ram);
  if (!memory || !size)
    return true;

  if (!ensure_directory(path_.parent_path(), error))
    return false;

  const auto temporary = path_.string() + ".tmp";
  {
    std::ofstream output(
        temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
      error = "cannot create save RAM temporary file";
      return false;
    }
    output.write(
        static_cast<const char*>(memory),
        static_cast<std::streamsize>(size));
    output.flush();
    if (!output) {
      error = "cannot write save RAM";
      return false;
    }
  }

  std::error_code ec;
  std::filesystem::rename(temporary, path_, ec);
  if (!ec)
    return true;

  // Some libc/filesystem combinations do not replace existing files on
  // rename. Remove the old file and retry while keeping the temporary intact.
  std::filesystem::remove(path_, ec);
  ec.clear();
  std::filesystem::rename(temporary, path_, ec);
  if (ec) {
    std::filesystem::remove(temporary);
    error = "cannot atomically replace save RAM";
    return false;
  }
  return true;
}

} // namespace native_emus::ps1
