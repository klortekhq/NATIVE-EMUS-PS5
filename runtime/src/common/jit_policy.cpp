#include <ps5rt/jit_policy.hpp>

#include <algorithm>
#include <limits>

namespace ps5rt {
namespace {

[[nodiscard]] bool power_of_two(
    std::size_t value) noexcept {
  return value != 0 &&
         (value & (value - 1)) == 0;
}

[[nodiscard]] bool aligned(
    std::size_t value,
    std::size_t alignment) noexcept {
  return value % alignment == 0;
}

[[nodiscard]] bool round_up_checked(
    std::size_t value,
    std::size_t alignment,
    std::size_t& out) noexcept {
  const std::size_t mask = alignment - 1;
  if (value >
      std::numeric_limits<std::size_t>::max() -
          mask) {
    return false;
  }
  out = (value + mask) & ~mask;
  return true;
}

} // namespace

bool valid_jit_cache_policy(
    const JitCachePolicy& policy) noexcept {
  if (!power_of_two(policy.alignment) ||
      policy.minimum_bytes == 0 ||
      policy.default_bytes == 0 ||
      policy.maximum_bytes == 0 ||
      policy.minimum_bytes >
          policy.default_bytes ||
      policy.default_bytes >
          policy.maximum_bytes) {
    return false;
  }

  return
      aligned(
          policy.minimum_bytes,
          policy.alignment) &&
      aligned(
          policy.default_bytes,
          policy.alignment) &&
      aligned(
          policy.maximum_bytes,
          policy.alignment);
}

bool select_jit_cache_size(
    const JitCachePolicy& policy,
    std::size_t override_bytes,
    JitCacheSelection& out) noexcept {
  out = {};

  if (!valid_jit_cache_policy(policy)) {
    return false;
  }

  const bool has_override =
      override_bytes != 0;
  const std::size_t requested =
      has_override
          ? override_bytes
          : policy.default_bytes;

  std::size_t bounded =
      std::clamp(
          requested,
          policy.minimum_bytes,
          policy.maximum_bytes);

  const bool clamped =
      bounded != requested;

  std::size_t selected = 0;
  if (!round_up_checked(
          bounded,
          policy.alignment,
          selected)) {
    return false;
  }

  if (selected >
      policy.maximum_bytes) {
    selected =
        policy.maximum_bytes;
  }

  out.bytes = selected;
  out.used_override = has_override;
  out.clamped = clamped;
  out.rounded = selected != bounded;
  return true;
}

} // namespace ps5rt
