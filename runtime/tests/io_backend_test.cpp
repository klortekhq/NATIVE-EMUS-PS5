#include <ps5rt/io.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace {

class MemoryReader final : public ps5rt::RandomAccessReader {
public:
  ps5rt::Result size(std::uint64_t& out) const noexcept override {
    out = bytes_.size();
    return ps5rt::Result::success();
  }

  ps5rt::Result read_at(
      std::uint64_t offset,
      std::span<std::byte> destination,
      std::size_t& out_read) noexcept override {
    out_read = 0;
    if (offset >= bytes_.size())
      return ps5rt::Result::success();

    const auto available = bytes_.size() - static_cast<std::size_t>(offset);
    const auto count = std::min(destination.size(), available);
    for (std::size_t i = 0; i < count; ++i)
      destination[i] = std::byte{bytes_[static_cast<std::size_t>(offset) + i]};
    out_read = count;
    return ps5rt::Result::success();
  }

private:
  static constexpr std::array<std::uint8_t, 8> bytes_{
      0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
};

ps5rt::Result mock_smb(
    std::string_view uri,
    ps5rt::OpenMode mode,
    ps5rt::RandomAccessReaderPtr& out) noexcept {
  if (mode != ps5rt::OpenMode::read_only ||
      uri != "smb://server/share/game.chd")
    return {ps5rt::ErrorCode::invalid_argument, 0, "unexpected mock SMB URI"};
  out = std::make_unique<MemoryReader>();
  return ps5rt::Result::success();
}

ps5rt::Result other_smb(
    std::string_view,
    ps5rt::OpenMode,
    ps5rt::RandomAccessReaderPtr&) noexcept {
  return {ps5rt::ErrorCode::system_error, 0, "wrong backend"};
}

} // namespace

int main() {
  using namespace ps5rt;

  assert(register_random_access_backend("smb", mock_smb));
  assert(register_random_access_backend("smb", mock_smb));
  assert(!register_random_access_backend("smb", other_smb));
  assert(!register_random_access_backend("SMB", mock_smb));

  RandomAccessReaderPtr reader;
  assert(open_random_access(
      "smb://server/share/game.chd",
      OpenMode::read_only,
      reader));
  assert(reader);

  std::uint64_t size = 0;
  assert(reader->size(size));
  assert(size == 8);

  std::array<std::byte, 3> got{};
  std::size_t count = 0;
  assert(reader->read_at(2, got, count));
  assert(count == 3);
  assert(got[0] == std::byte{0x30});
  assert(got[1] == std::byte{0x40});
  assert(got[2] == std::byte{0x50});

  assert(!unregister_random_access_backend("smb", other_smb));
  assert(unregister_random_access_backend("smb", mock_smb));

  reader.reset();
  assert(!open_random_access(
      "smb://server/share/game.chd",
      OpenMode::read_only,
      reader));

  return 0;
}
