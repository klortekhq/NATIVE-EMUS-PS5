#include <ps5rt/memory.hpp>

#include <array>
#include <cstddef>
#include <cstdio>

int main() {
  ps5rt::MemoryRequest request{};
  request.size = 3ull * 1024 * 1024;
  request.alignment = 2ull * 1024 * 1024;
  request.protection =
      ps5rt::Protection::read |
      ps5rt::Protection::write;
  request.debug_name = "ps5rt-pool-probe";

  ps5rt::Mapping mapping{};
  const auto allocated = ps5rt::allocate_memory(
      ps5rt::MemoryKind::pooled,
      request,
      mapping);
  if (!allocated || !mapping) {
    std::printf(
        "ps5rt-pool-memory-probe: allocate failed code=%d native=%d\n",
        static_cast<int>(allocated.code),
        allocated.native_code);
    return 1;
  }

  std::array<std::byte, 64> pattern{};
  for (std::size_t i = 0; i < pattern.size(); ++i)
    pattern[i] = static_cast<std::byte>(i ^ 0x5au);

  auto* bytes = static_cast<std::byte*>(mapping.address);
  for (std::size_t i = 0; i < pattern.size(); ++i)
    bytes[i] = pattern[i];
  for (std::size_t i = 0; i < pattern.size(); ++i) {
    if (bytes[i] != pattern[i]) {
      std::printf("ps5rt-pool-memory-probe: verify failed\n");
      return 2;
    }
  }

  const auto released = ps5rt::release_memory(mapping);
  if (!released || mapping) {
    std::printf(
        "ps5rt-pool-memory-probe: release failed code=%d native=%d\n",
        static_cast<int>(released.code),
        released.native_code);
    return 3;
  }

  request.size = 1ull * 1024 * 1024;
  const auto reused = ps5rt::allocate_memory(
      ps5rt::MemoryKind::pooled,
      request,
      mapping);
  if (!reused || !mapping) {
    std::printf(
        "ps5rt-pool-memory-probe: reuse failed code=%d native=%d\n",
        static_cast<int>(reused.code),
        reused.native_code);
    return 4;
  }

  const auto released_again =
      ps5rt::release_memory(mapping);
  if (!released_again || mapping)
    return 5;

  std::printf("ps5rt-pool-memory-probe: PASS\n");
  return 0;
}
