#include <ps5rt/vfs.hpp>

#include <cerrno>
#include <cstddef>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace ps5rt {
namespace {

bool is_directory(std::string_view path) noexcept {
  struct stat st {};
  const std::string p(path);
  return ::stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool writable(std::string_view path) noexcept {
  const std::string p(path);
  return ::access(p.c_str(), W_OK) == 0;
}

Result mkdir_one(const std::string& path) noexcept {
  if (path.empty() || path == "/")
    return Result::success();

  if (::mkdir(path.c_str(), 0775) == 0)
    return Result::success();

  const int mkdir_error = errno;
  if (mkdir_error != EEXIST)
    return {ErrorCode::io_error, mkdir_error, "mkdir failed"};

  // EEXIST is success only when the existing object is actually a directory.
  // Treating a regular file as a directory makes the error surface depend on
  // whether another path component happens to follow it.
  struct stat st {};
  if (::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
    return Result::success();

  const int stat_error = errno;
  return {
      ErrorCode::io_error,
      stat_error != 0 ? stat_error : ENOTDIR,
      "path exists and is not a directory"};
}

} // namespace

Result ensure_directory(std::string_view path) noexcept {
  if (path.empty())
    return {ErrorCode::invalid_argument, 0, "empty directory path"};

  std::string normalized;
  const auto nr = normalize_host_path(path, normalized);
  if (!nr)
    return nr;

  std::string current;
  current.reserve(normalized.size());

  if (!normalized.empty() && normalized.front() == '/')
    current = "/";

  std::size_t begin = normalized.front() == '/' ? 1u : 0u;
  while (begin <= normalized.size()) {
    const auto slash = normalized.find('/', begin);
    const auto end = slash == std::string::npos ? normalized.size() : slash;
    const auto part = normalized.substr(begin, end - begin);

    if (!part.empty()) {
      if (!current.empty() && current.back() != '/')
        current.push_back('/');
      current.append(part);
      const auto r = mkdir_one(current);
      if (!r)
        return r;
    }

    if (slash == std::string::npos)
      break;
    begin = slash + 1;
  }

  return Result::success();
}

Result normalize_host_path(std::string_view input, std::string& out) noexcept {
  out.clear();
  if (input.empty())
    return {ErrorCode::invalid_argument, 0, "empty host path"};

  const bool absolute = input.front() == '/';
  std::vector<std::string_view> parts;

  std::size_t pos = 0;
  while (pos <= input.size()) {
    const auto slash = input.find('/', pos);
    const auto end = slash == std::string_view::npos ? input.size() : slash;
    const auto part = input.substr(pos, end - pos);

    if (part.empty() || part == ".") {
      // Ignore.
    } else if (part == "..") {
      if (!parts.empty() && parts.back() != "..")
        parts.pop_back();
      else if (!absolute)
        parts.push_back(part);
    } else {
      parts.push_back(part);
    }

    if (slash == std::string_view::npos)
      break;
    pos = slash + 1;
  }

  if (absolute)
    out.push_back('/');

  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i != 0)
      out.push_back('/');
    out.append(parts[i]);
  }

  if (out.empty())
    out = absolute ? "/" : ".";

  return Result::success();
}

Result enumerate_storage(std::vector<StorageRoot>& out) noexcept {
  out.clear();

  struct Candidate {
    StorageKind kind;
    const char* path;
  };

  // Only expose roots that really exist on the running console. Mount names
  // vary across jailbreak/storage setups, so absence is not treated as error.
  // /app0 is the read-only application image. Current PS5SX2 platform code
  // probes removable storage across /mnt/usb0..7, so keep the common runtime
  // broad enough for all eight observed USB mount slots instead of silently
  // ignoring drives above usb1.
  static constexpr Candidate candidates[] = {
      {StorageKind::app, "/app0"},
      {StorageKind::data, "/data"},
      {StorageKind::usb, "/mnt/usb0"},
      {StorageKind::usb, "/mnt/usb1"},
      {StorageKind::usb, "/mnt/usb2"},
      {StorageKind::usb, "/mnt/usb3"},
      {StorageKind::usb, "/mnt/usb4"},
      {StorageKind::usb, "/mnt/usb5"},
      {StorageKind::usb, "/mnt/usb6"},
      {StorageKind::usb, "/mnt/usb7"},
      {StorageKind::internal, "/mnt/ext0"},
      {StorageKind::m2, "/mnt/ext1"},
  };

  for (const auto& candidate : candidates) {
    if (!is_directory(candidate.path))
      continue;
    out.push_back({candidate.kind, candidate.path, writable(candidate.path)});
  }

  return Result::success();
}

} // namespace ps5rt
