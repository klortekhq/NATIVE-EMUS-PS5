#include "emu_server_identity.hpp"

#include <cassert>

int main() {
  using namespace ps5rt::detail;

  EmuServerObjectIdentity identity;
  assert(!identity.initialized());

  assert(
      identity.observe(4096, "\"etag-a\"", "bytes") ==
      EmuServerIdentityResult::initialized);
  assert(identity.initialized());
  assert(identity.size() == 4096);
  assert(identity.etag() == "\"etag-a\"");

  assert(
      identity.observe(4096, "\"etag-a\"", "BYTES") ==
      EmuServerIdentityResult::unchanged);

  assert(
      identity.observe(4097, "\"etag-a\"", "bytes") ==
      EmuServerIdentityResult::changed);
  assert(
      identity.observe(4096, "\"etag-b\"", "bytes") ==
      EmuServerIdentityResult::changed);

  EmuServerObjectIdentity missing_etag;
  assert(
      missing_etag.observe(4096, "", "bytes") ==
      EmuServerIdentityResult::invalid_contract);
  assert(!missing_etag.initialized());

  EmuServerObjectIdentity no_ranges;
  assert(
      no_ranges.observe(4096, "\"etag-a\"", "none") ==
      EmuServerIdentityResult::invalid_contract);
  assert(!no_ranges.initialized());

  return 0;
}
