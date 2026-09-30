#include <ps5rt/io.hpp>

#include <cstdio>
#include <memory>
#include <string>

namespace ps5rt {
namespace {

class FileReader final : public RandomAccessReader {
public:
  FileReader(std::FILE* file, std::uint64_t size) noexcept : file_(file), size_(size) {}
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
    if (std::fseek(file_, static_cast<long>(offset), SEEK_SET) != 0)
      return {ErrorCode::io_error, 0, "fseek failed"};
    out_read = std::fread(destination.data(), 1, destination.size(), file_);
    if (out_read != destination.size() && std::ferror(file_))
      return {ErrorCode::io_error, 0, "fread failed"};
    return Result::success();
  }

private:
  std::FILE* file_{};
  std::uint64_t size_{};
};

std::string local_path(std::string_view uri) {
  constexpr std::string_view prefix = "file://";
  if (uri.substr(0, prefix.size()) == prefix)
    return std::string(uri.substr(prefix.size()));
  return std::string(uri);
}

} // namespace

Result open_random_access(
    std::string_view uri,
    OpenMode mode,
    RandomAccessReaderPtr& out) noexcept {
  out.reset();
  if (uri.empty())
    return {ErrorCode::invalid_argument, 0, "empty content URI"};
  if (mode != OpenMode::read_only)
    return {ErrorCode::unsupported, 0, "read-write random access not implemented"};

  const std::string path = local_path(uri);
  if (path.find("://") != std::string::npos)
    return {ErrorCode::unsupported, 0, "URI scheme not handled by local backend"};

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

  out = std::make_unique<FileReader>(f, static_cast<std::uint64_t>(n));
  return Result::success();
}

} // namespace ps5rt
