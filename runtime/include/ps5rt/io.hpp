#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

#include <ps5rt/result.hpp>

namespace ps5rt {

enum class OpenMode : std::uint8_t {
  read_only,
  read_write,
};

class RandomAccessReader {
public:
  virtual ~RandomAccessReader() = default;

  RandomAccessReader(const RandomAccessReader&) = delete;
  RandomAccessReader& operator=(const RandomAccessReader&) = delete;

  [[nodiscard]] virtual Result size(std::uint64_t& out_bytes) const noexcept = 0;

  virtual Result read_at(
      std::uint64_t offset,
      std::span<std::byte> destination,
      std::size_t& out_read) noexcept = 0;

protected:
  RandomAccessReader() = default;
};

using RandomAccessReaderPtr = std::unique_ptr<RandomAccessReader>;

using RandomAccessOpenFn = Result(*)(
    std::string_view uri,
    OpenMode mode,
    RandomAccessReaderPtr& out) noexcept;

// Register a URI backend such as "smb" or "nfs". The scheme is stored by
// value; the callback must remain valid until unregistered. Built-in local
// file:// and usb:// handling does not use this registry.
Result register_random_access_backend(
    std::string_view scheme,
    RandomAccessOpenFn opener) noexcept;

Result unregister_random_access_backend(
    std::string_view scheme,
    RandomAccessOpenFn opener) noexcept;

// URI examples:
//   file:///data/roms/game.iso
//   usb:///mnt/usb0/game.chd
//   smb://server/share/path/game.iso
//
// Credentials must be supplied by the configured host backend rather than
// embedded into emulator manifests or command lines.
Result open_random_access(
    std::string_view uri,
    OpenMode mode,
    RandomAccessReaderPtr& out) noexcept;

} // namespace ps5rt
