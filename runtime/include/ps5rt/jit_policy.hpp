#pragma once

#include <cstddef>

namespace ps5rt {

struct JitCachePolicy {
  std::size_t default_bytes{};
  std::size_t minimum_bytes{};
  std::size_t maximum_bytes{};
  std::size_t alignment{0x4000};
};

struct JitCacheSelection {
  std::size_t bytes{};
  bool used_override{};
  bool clamped{};
  bool rounded{};
};

[[nodiscard]] bool valid_jit_cache_policy(
    const JitCachePolicy& policy) noexcept;

[[nodiscard]] bool select_jit_cache_size(
    const JitCachePolicy& policy,
    std::size_t override_bytes,
    JitCacheSelection& out) noexcept;

} // namespace ps5rt
