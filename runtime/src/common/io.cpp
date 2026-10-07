#include <ps5rt/io.hpp>

#include <array>
#include <cstdio>
#include <limits>
#include <memory>
#include <mutex>
#include <string>

namespace ps5rt {
namespace {

struct BackendSlot {
  std::string scheme{};
  RandomAccessOpenFn opener{};
};

std::mutex g_backend_mutex;
std::array<BackendSlot, 8> g_backends{};

bool valid_scheme(std::string_view scheme) noexcept {
  if (scheme.empty() || scheme.size() > 15)
    return false;
  for (const char ch : scheme) {
    const bool ok =
        (ch >= 'a' && ch <= 'z') ||
        (ch >= '0' && ch <= '9') ||
        ch == '+' || ch == '-' || ch == '.';
    if (!ok)
      return false;
  }
  return true;
}

std::string_view uri_scheme(std::string_view uri) noexcept {
  const auto separator = uri.find("://");
  if (separator == std::string_view::npos)
    return {};
  return uri.substr(0, separator);
}

RandomAccessOpenFn find_backend(std::string_view scheme) noexcept {
  std::scoped_lock lock(g_backend_mutex);
  for (const auto& slot : g_backends) {
    if (slot.opener && slot.scheme == scheme)
      return slot.opener;
  }
  return nullptr;
}

class FileReader final : public RandomAccessReader {
public:
  FileReader(std::FILE* file, std::uint64_t size) noexcept
      : file_(file), size_(size) {}

  ~FileReader() override {
    if (file_)
      std::fclose(file_);
  }

  Result size(std::uint64_t& out_bytes) const noexcept override {
    out_bytes = size_;
    return Result::success();
  }

  Result read_at(
      std::uint64_t offset,
      std::span<std::byte> destination,
      std::size_t& out_read) noexcept override {
    out_read = 0;
    if (!file_ || offset > size_)
      return {ErrorCode::invalid_argument, 0, "invalid file read offset"};

    // The public PS5 libc uses a long large enough for the title ABI today,
    // but keep the conversion checked so the common backend is also safe on
    // host platforms where long may be narrower.
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<long>::max()))
      return {ErrorCode::unsupported, 0, "file offset exceeds stdio range"};

    if (std::fseek(file_, static_cast<long>(offset), SEEK_SET) != 0)
      return {ErrorCode::io_error, 0, "fseek failed"};

    out_read = std::fread(
        destination.data(), 1, destination.size(), file_);
    if (out_read != destination.size() && std::ferror(file_))
      return {ErrorCode::io_error, 0, "fread failed"};
    return Result::success();
  }

private:
  std::FILE* file_{};
  std::uint64_t size_{};
};

std::string local_path(std::string_view uri) {
  constexpr std::string_view file_prefix = "file://";
  constexpr std::string_view usb_prefix = "usb://";

  if (uri.substr(0, file_prefix.size()) == file_prefix)
    return std::string(uri.substr(file_prefix.size()));

  // usb:// is only a semantic hint; the URI still carries an absolute PS5
  // mount path such as usb:///mnt/usb0/game.chd.
  if (uri.substr(0, usb_prefix.size()) == usb_prefix)
    return std::string(uri.substr(usb_prefix.size()));

  return std::string(uri);
}

} // namespace

Result register_random_access_backend(
    std::string_view scheme,
    RandomAccessOpenFn opener) noexcept {
  if (!valid_scheme(scheme) || !opener)
    return {ErrorCode::invalid_argument, 0, "invalid random-access backend"};

  std::scoped_lock lock(g_backend_mutex);

  for (auto& slot : g_backends) {
    if (slot.opener && slot.scheme == scheme) {
      if (slot.opener == opener)
        return Result::success();
      return {ErrorCode::system_error, 0, "URI backend already registered"};
    }
  }

  for (auto& slot : g_backends) {
    if (!slot.opener) {
      slot.scheme.assign(scheme);
      slot.opener = opener;
      return Result::success();
    }
  }

  return {ErrorCode::out_of_memory, 0, "URI backend registry full"};
}

Result unregister_random_access_backend(
    std::string_view scheme,
    RandomAccessOpenFn opener) noexcept {
  if (!valid_scheme(scheme) || !opener)
    return {ErrorCode::invalid_argument, 0, "invalid random-access backend"};

  std::scoped_lock lock(g_backend_mutex);
  for (auto& slot : g_backends) {
    if (slot.opener && slot.scheme == scheme) {
      if (slot.opener != opener)
        return {ErrorCode::invalid_argument, 0, "URI backend owner mismatch"};
      slot = {};
      return Result::success();
    }
  }

  return {ErrorCode::invalid_argument, 0, "URI backend not registered"};
}

Result open_random_access(
    std::string_view uri,
    OpenMode mode,
    RandomAccessReaderPtr& out) noexcept {
  out.reset();

  if (uri.empty())
    return {ErrorCode::invalid_argument, 0, "empty content URI"};

  if (mode != OpenMode::read_only)
    return {
        ErrorCode::unsupported, 0,
        "read-write random access not implemented"};

  const auto scheme = uri_scheme(uri);
  if (!scheme.empty() && scheme != "file" && scheme != "usb") {
    if (const auto opener = find_backend(scheme))
      return opener(uri, mode, out);
    return {
        ErrorCode::unsupported, 0,
        "URI scheme not handled by registered backend"};
  }

  const std::string path = local_path(uri);
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f)
    return {ErrorCode::io_error, 0, "fopen failed"};

  if (std::fseek(f, 0, SEEK_END) != 0) {
    std::fclose(f);
    return {ErrorCode::io_error, 0, "fseek end failed"};
  }

  const long n = std::ftell(f);
  if (n < 0 || std::fseek(f, 0, SEEK_SET) != 0) {
    std::fclose(f);
    return {ErrorCode::io_error, 0, "ftell failed"};
  }

  out = std::make_unique<FileReader>(
      f, static_cast<std::uint64_t>(n));
  return Result::success();
}

} // namespace ps5rt
