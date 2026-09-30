#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <ps5rt/result.hpp>

namespace ps5rt {

struct ThreadConfig {
  const char* name{};
  std::size_t stack_size{};
  std::int32_t priority{};
  std::int32_t affinity_hint{-1};
};

Result set_current_thread_name(const char* name) noexcept;
Result set_current_thread_affinity(std::int32_t cpu) noexcept;
Result query_current_thread_stack(std::size_t& out_bytes) noexcept;

// Used by ports that need to override small host/default stacks without
// replacing std::thread throughout an upstream core.
[[nodiscard]] std::size_t recommended_stack_size(
    std::string_view subsystem,
    std::size_t upstream_default) noexcept;

} // namespace ps5rt
