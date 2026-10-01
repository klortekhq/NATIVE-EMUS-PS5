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

std::string_view final_component(std::string_view input) noexcept {
  const auto slash = input.find_last_of("/\\");
  return slash == std::string_view::npos ? input : input.substr(slash + 1);
}

bool is_emus_uri(std::string_view input) noexcept {
  constexpr std::string_view prefix = "emus://";
  return input.substr(0, prefix.size()) == prefix;
}

std::string save_stem(std::filesystem::path path) {
  auto stem = path.stem().string();
  if (stem.empty())
    stem = "ps1-content";
  return stem;
}

std::string trim(std::string value) {
  const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
  const auto first = std::find_if(value.begin(), value.end(), not_space);
  if (first == value.end())
    return {};
  const auto last = std::find_if(value.rbegin(), value.rend(), not_space).base();
  return std::string(first, last);
}

bool validate_local_m3u(
    const std::filesystem::path& playlist,
    std::vector<std::filesystem::path>& entries,
    std::string& error) {
  std::ifstream input(playlist);
  if (!input) {
    error = "cannot open PS1 M3U playlist";
    return false;
  }

  const auto base = playlist.parent_path();
  std::string line;
  while (std::getline(input, line)) {
    line = trim(std::move(line));
    if (line.empty() || line.front() == '#')
      continue;

    const auto scheme = line.find("://");
    if (scheme != std::string::npos) {
      error = "PS1 M3U network entries require the ps5rt VFS bridge";
      return false;
    }

    std::filesystem::path entry(line);
    if (entry.is_relative())
      entry = base / entry;
    entry = entry.lexically_normal();

    if (lowercase(entry.extension().string()) == ".m3u") {
      error = "nested PS1 M3U playlists are not supported";
      return false;
    }
    if (!is_supported_ps1_content(entry.string())) {
      error = "PS1 M3U contains unsupported content";
      return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(entry, ec) || ec ||
        !std::filesystem::is_regular_file(entry, ec) || ec) {
      error = "PS1 M3U entry does not exist";
      return false;
    }
    entries.push_back(std::move(entry));
  }

  if (entries.empty()) {
    error = "PS1 M3U playlist has no discs";
    return false;
  }
  return true;
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
  state_dir = data_root / "states";
}

bool is_supported_ps1_content(std::string_view path) noexcept {
  if (path.empty())
    return false;

  const std::string local_or_name =
      is_emus_uri(path)
          ? std::string(final_component(path))
          : strip_file_uri(path);
  std::filesystem::path parsed(local_or_name);
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
  const bool remote_emus = is_emus_uri(uri_or_path);
  if (scheme != std::string_view::npos &&
      uri_or_path.substr(0, scheme) != "file" &&
      !remote_emus) {
    error = "unsupported PS1 URI scheme";
    return false;
  }

  if (!is_supported_ps1_content(uri_or_path)) {
    error = "unsupported PS1 content extension";
    return false;
  }

  if (!ensure_directory(layout.system_dir, error) ||
      !ensure_directory(layout.save_dir, error) ||
      !ensure_directory(layout.state_dir, error))
    return false;

  if (remote_emus) {
    const std::string name(final_component(uri_or_path));
    std::filesystem::path metadata_name(name);
    if (lowercase(metadata_name.extension().string()) == ".m3u") {
      error = "remote PS1 M3U playlists are not supported yet";
      return false;
    }

    out.core_path = std::string(uri_or_path);
    const auto stem = save_stem(metadata_name);
    out.save_ram_path = layout.save_dir / (stem + ".srm");
    out.state_path = layout.state_dir / (stem + ".state0");
    out.local_file = false;
    return true;
  }

  const std::string local = strip_file_uri(uri_or_path);
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

  if (lowercase(content.extension().string()) == ".m3u" &&
      !validate_local_m3u(content, out.playlist_entries, error))
    return false;

  out.core_path = content.string();
  const auto stem = save_stem(content);
  out.save_ram_path = layout.save_dir / (stem + ".srm");
  out.state_path = layout.state_dir / (stem + ".state0");
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

SaveStateStore::SaveStateStore(std::filesystem::path path)
    : path_(std::move(path)) {}

bool SaveStateStore::save(
    corehost::StaticCore& core,
    std::string& error) const {
  error.clear();

  std::vector<std::uint8_t> state;
  if (!core.save_state(state) || state.empty()) {
    error = "core could not serialize PS1 save state";
    return false;
  }

  constexpr std::size_t max_state_size = 128u * 1024u * 1024u;
  if (state.size() > max_state_size) {
    error = "PS1 save state exceeds safety limit";
    return false;
  }

  if (!ensure_directory(path_.parent_path(), error))
    return false;

  constexpr std::array<std::uint8_t, 8> magic{
      'N','E','P','S','1','S','T',0};
  constexpr std::uint32_t version = 1;
  const std::uint64_t payload_size = state.size();

  const auto temporary = path_.string() + ".tmp";
  {
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
      error = "cannot create PS1 save-state temporary file";
      return false;
    }

    output.write(
        reinterpret_cast<const char*>(magic.data()),
        static_cast<std::streamsize>(magic.size()));
    output.write(
        reinterpret_cast<const char*>(&version),
        static_cast<std::streamsize>(sizeof(version)));
    output.write(
        reinterpret_cast<const char*>(&payload_size),
        static_cast<std::streamsize>(sizeof(payload_size)));
    output.write(
        reinterpret_cast<const char*>(state.data()),
        static_cast<std::streamsize>(state.size()));
    output.flush();

    if (!output) {
      error = "cannot write PS1 save state";
      return false;
    }
  }

  std::error_code ec;
  std::filesystem::rename(temporary, path_, ec);
  if (!ec)
    return true;

  std::filesystem::remove(path_, ec);
  ec.clear();
  std::filesystem::rename(temporary, path_, ec);
  if (ec) {
    std::filesystem::remove(temporary);
    error = "cannot atomically replace PS1 save state";
    return false;
  }
  return true;
}

bool SaveStateStore::load(
    corehost::StaticCore& core,
    std::string& error) const {
  error.clear();

  std::error_code ec;
  if (!std::filesystem::exists(path_, ec)) {
    if (ec)
      error = "cannot stat PS1 save state";
    else
      error = "PS1 save state slot 0 does not exist";
    return false;
  }

  std::ifstream input(path_, std::ios::binary);
  if (!input) {
    error = "cannot open PS1 save state";
    return false;
  }

  constexpr std::array<std::uint8_t, 8> expected_magic{
      'N','E','P','S','1','S','T',0};
  std::array<std::uint8_t, 8> magic{};
  std::uint32_t version{};
  std::uint64_t payload_size{};

  input.read(
      reinterpret_cast<char*>(magic.data()),
      static_cast<std::streamsize>(magic.size()));
  input.read(
      reinterpret_cast<char*>(&version),
      static_cast<std::streamsize>(sizeof(version)));
  input.read(
      reinterpret_cast<char*>(&payload_size),
      static_cast<std::streamsize>(sizeof(payload_size)));

  if (!input || magic != expected_magic || version != 1) {
    error = "invalid PS1 save-state header";
    return false;
  }

  constexpr std::uint64_t max_state_size = 128ull * 1024ull * 1024ull;
  if (payload_size == 0 || payload_size > max_state_size) {
    error = "invalid PS1 save-state payload size";
    return false;
  }

  input.seekg(0, std::ios::end);
  const auto end = input.tellg();
  constexpr std::uint64_t header_size =
      8u + sizeof(std::uint32_t) + sizeof(std::uint64_t);
  if (end < 0 ||
      static_cast<std::uint64_t>(end) != header_size + payload_size) {
    error = "PS1 save-state file is truncated or has trailing data";
    return false;
  }

  input.seekg(static_cast<std::streamoff>(header_size), std::ios::beg);
  std::vector<std::uint8_t> state(static_cast<std::size_t>(payload_size));
  input.read(
      reinterpret_cast<char*>(state.data()),
      static_cast<std::streamsize>(state.size()));
  if (!input) {
    error = "cannot read PS1 save-state payload";
    return false;
  }

  if (!core.load_state(state.data(), state.size())) {
    error = "core rejected PS1 save state";
    return false;
  }
  return true;
}

} // namespace native_emus::ps1
