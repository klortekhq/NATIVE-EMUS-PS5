#include "smb_read_ahead.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

int main() {
  using ps5rt::detail::SmbReadAheadCache;

  {
    SmbReadAheadCache disabled(0);
    assert(disabled.fetch_size(4096, 64 * 1024) == 4096);
  }

  {
    SmbReadAheadCache cache(512 * 1024);
    assert(cache.capacity_bytes() == 512 * 1024);
    assert(cache.fetch_size(4096, 2 * 1024 * 1024) == 512 * 1024);
    assert(cache.fetch_size(768 * 1024, 2 * 1024 * 1024) == 768 * 1024);
    assert(cache.fetch_size(4096, 128 * 1024) == 128 * 1024);

    std::vector<std::byte> bytes(16);
    for (std::size_t i = 0; i < bytes.size(); ++i)
      bytes[i] = static_cast<std::byte>(i);
    cache.store(100, std::move(bytes), 16);

    std::array<std::byte, 4> out{};
    std::size_t read = 0;
    assert(cache.copy(104, out, out.size(), read));
    assert(read == out.size());
    assert(out[0] == std::byte{4});
    assert(out[3] == std::byte{7});

    assert(!cache.copy(96, out, out.size(), read));
    assert(!cache.copy(114, out, out.size(), read));

    cache.clear();
    assert(!cache.copy(104, out, out.size(), read));
  }

  return 0;
}
