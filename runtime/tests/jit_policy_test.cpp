#include <ps5rt/jit_policy.hpp>

#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>

int main() {
  constexpr std::size_t mib =
      1024u * 1024u;

  const ps5rt::JitCachePolicy policy{
      .default_bytes = 64u * mib,
      .minimum_bytes = 16u * mib,
      .maximum_bytes = 256u * mib,
      .alignment = 0x4000,
  };

  assert(ps5rt::valid_jit_cache_policy(policy));

  ps5rt::JitCacheSelection selected{};
  assert(ps5rt::select_jit_cache_size(
      policy, 0, selected));
  assert(selected.bytes == 64u * mib);
  assert(!selected.used_override);
  assert(!selected.clamped);
  assert(!selected.rounded);

  assert(ps5rt::select_jit_cache_size(
      policy, 8u * mib, selected));
  assert(selected.bytes == 16u * mib);
  assert(selected.used_override);
  assert(selected.clamped);

  assert(ps5rt::select_jit_cache_size(
      policy, 512u * mib, selected));
  assert(selected.bytes == 256u * mib);
  assert(selected.clamped);

  assert(ps5rt::select_jit_cache_size(
      policy,
      32u * mib + 1u,
      selected));
  assert(selected.bytes ==
         32u * mib + 0x4000u);
  assert(!selected.clamped);
  assert(selected.rounded);

  auto invalid = policy;
  invalid.alignment = 24;
  assert(!ps5rt::valid_jit_cache_policy(
      invalid));
  assert(!ps5rt::select_jit_cache_size(
      invalid, 0, selected));

  invalid = policy;
  invalid.minimum_bytes =
      16u * mib + 1u;
  assert(!ps5rt::valid_jit_cache_policy(
      invalid));

  invalid = policy;
  invalid.default_bytes =
      8u * mib;
  assert(!ps5rt::valid_jit_cache_policy(
      invalid));

  invalid = policy;
  invalid.maximum_bytes =
      std::numeric_limits<std::size_t>::max();
  assert(!ps5rt::valid_jit_cache_policy(
      invalid));

  std::cout
      << "ps5rt JIT cache policy tests passed\n";
  return 0;
}
