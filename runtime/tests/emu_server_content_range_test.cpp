#include "emu_server_content_range.hpp"

#include <cassert>
#include <cstdint>
#include <limits>

int main() {
  using namespace ps5rt::detail;

  EmuServerContentRange range{};
  assert(parse_emu_server_content_range(
      "bytes 2-5/10", range));
  assert(range.first == 2);
  assert(range.last == 5);
  assert(range.total == 10);

  assert(parse_emu_server_content_range(
      " BYTES 0-0/1\t", range));
  assert(range.first == 0);
  assert(range.last == 0);
  assert(range.total == 1);

  assert(matches_emu_server_content_range(
      "bytes 1048576-1572863/3135105024",
      1048576,
      1572863,
      3135105024ull));
  assert(!matches_emu_server_content_range(
      "bytes 1048577-1572863/3135105024",
      1048576,
      1572863,
      3135105024ull));
  assert(!matches_emu_server_content_range(
      "bytes 1048576-1572863/3135105025",
      1048576,
      1572863,
      3135105024ull));

  for (const auto invalid : {
           "",
           "bytes */10",
           "bytes 0-9/*",
           "items 0-9/10",
           "bytes 9-0/10",
           "bytes 0-10/10",
           "bytes 0-/10",
           "bytes -9/10",
           "bytes 0-9/",
           "bytes 0-9/10,20-29/30",
           "bytes 18446744073709551616-18446744073709551616/18446744073709551616",
       }) {
    assert(!parse_emu_server_content_range(invalid, range));
  }

  const auto max = std::numeric_limits<std::uint64_t>::max();
  assert(parse_emu_server_content_range(
      "bytes 18446744073709551614-18446744073709551614/18446744073709551615",
      range));
  assert(range.first == max - 1);
  assert(range.last == max - 1);
  assert(range.total == max);

  return 0;
}
