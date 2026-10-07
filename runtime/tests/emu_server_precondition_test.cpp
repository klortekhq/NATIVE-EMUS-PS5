#include "emu_server_precondition.hpp"

#include <cassert>

int main() {
  using namespace ps5rt::detail;

  static_assert(kEmuServerEntityPreconditionHeader == "If-Match");
  static_assert(
      classify_emu_server_range_status(206) ==
      EmuServerRangeStatus::partial_content);
  static_assert(
      classify_emu_server_range_status(412) ==
      EmuServerRangeStatus::stale_object);
  static_assert(
      classify_emu_server_range_status(200) ==
      EmuServerRangeStatus::unexpected);
  static_assert(
      classify_emu_server_range_status(500) ==
      EmuServerRangeStatus::unexpected);

  assert(kEmuServerEntityPreconditionHeader == "If-Match");
  return 0;
}
