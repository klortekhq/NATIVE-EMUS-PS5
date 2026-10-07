#pragma once

#include <cstddef>

#include <ps5rt/memory.hpp>
#include <ps5rt/result.hpp>

namespace ps5rt {

struct JitRegion {
  // Some backends may expose separate write and execute aliases. When PS5
  // maps one RWX view, both mappings may describe the same address.
  Mapping write_view{};
  Mapping execute_view{};

  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return static_cast<bool>(execute_view);
  }
};

struct JitRequest {
  std::size_t size{};
  std::size_t alignment{};
  const char* debug_name{};
  bool prefer_dual_mapping{true};

  // Optional address controls for backends whose generated-code ABI depends
  // on low/fixed host ranges (Xenia is one such case). These are ignored when
  // null unless the corresponding require_fixed_* flag is set.
  void* preferred_write_address{};
  void* preferred_execute_address{};
  bool require_fixed_write{false};
  bool require_fixed_execute{false};
};

Result create_jit_region(const JitRequest& request, JitRegion& out) noexcept;
Result destroy_jit_region(JitRegion& region) noexcept;
Result flush_instruction_cache(void* address, std::size_t size) noexcept;

} // namespace ps5rt
