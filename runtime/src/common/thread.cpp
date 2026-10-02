#include <ps5rt/thread.hpp>

#include <algorithm>

namespace ps5rt {
namespace {

constexpr std::size_t kEmulationThreadMinimum =
    2u * 1024u * 1024u;

[[nodiscard]] bool contains_token(
    std::string_view value,
    std::string_view token) noexcept {
  return value.find(token) != std::string_view::npos;
}

} // namespace

std::size_t recommended_stack_size(
    std::string_view subsystem,
    std::size_t upstream_default) noexcept {
  const bool heavy_guest_execution =
      contains_token(subsystem, "jit") ||
      contains_token(subsystem, "recompiler") ||
      contains_token(subsystem, "emulation") ||
      contains_token(subsystem, "emulator");

  if (!heavy_guest_execution)
    return upstream_default;

  return std::max(
      upstream_default,
      kEmulationThreadMinimum);
}

} // namespace ps5rt
