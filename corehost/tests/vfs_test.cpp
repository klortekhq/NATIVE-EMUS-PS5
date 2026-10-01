#include <corehost/vfs.hpp>

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

int main() {
  const auto path =
      std::filesystem::temp_directory_path() /
      "native-emus-corehost-vfs.bin";

  std::vector<std::uint8_t> expected(8192);
  for (std::size_t i = 0; i < expected.size(); ++i)
    expected[i] = static_cast<std::uint8_t>((i * 37u) & 0xffu);

  {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(
        reinterpret_cast<const char*>(expected.data()),
        static_cast<std::streamsize>(expected.size()));
  }

  auto* vfs = corehost::vfs_interface_v1();
  assert(vfs);
  assert(vfs->open);
  assert(vfs->read);
  assert(vfs->seek);

  auto* h = vfs->open(
      path.string().c_str(),
      corehost::lr::vfs_file_access_read,
      0);
  assert(h);
  assert(std::string(vfs->get_path(h)) == path.string());
  assert(vfs->size(h) == static_cast<std::int64_t>(expected.size()));
  assert(vfs->tell(h) == 0);

  std::uint8_t first[17]{};
  assert(vfs->read(h, first, sizeof(first)) ==
         static_cast<std::int64_t>(sizeof(first)));
  for (std::size_t i = 0; i < sizeof(first); ++i)
    assert(first[i] == expected[i]);
  assert(vfs->tell(h) == 17);

  assert(vfs->seek(h, 2048, corehost::lr::vfs_seek_start) == 0);
  std::uint8_t middle[33]{};
  assert(vfs->read(h, middle, sizeof(middle)) ==
         static_cast<std::int64_t>(sizeof(middle)));
  for (std::size_t i = 0; i < sizeof(middle); ++i)
    assert(middle[i] == expected[2048 + i]);

  assert(vfs->seek(h, -16, corehost::lr::vfs_seek_end) == 0);
  assert(vfs->tell(h) ==
         static_cast<std::int64_t>(expected.size() - 16));

  std::uint8_t tail[32]{};
  assert(vfs->read(h, tail, sizeof(tail)) == 16);
  for (std::size_t i = 0; i < 16; ++i)
    assert(tail[i] == expected[expected.size() - 16 + i]);
  assert(vfs->read(h, tail, 1) == 0);

  // Read-only contract: opening/writing with write access is refused.
  assert(!vfs->open(
      path.string().c_str(),
      corehost::lr::vfs_file_access_read_write,
      0));
  assert(vfs->write(h, tail, sizeof(tail)) == -1);

  assert(vfs->close(h) == 0);
  std::filesystem::remove(path);
  return 0;
}
