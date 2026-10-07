#pragma once

#include <corehost/libretro_abi.hpp>

namespace corehost {

// Returns the libretro ABI exported by the single core statically linked into
// the executable. Each PS5 app links exactly one such core.
[[nodiscard]] lr::StaticApi linked_core_api() noexcept;

} // namespace corehost
