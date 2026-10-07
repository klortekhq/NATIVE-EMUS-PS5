#pragma once

#include <corehost/libretro_abi.hpp>

namespace corehost {

// Read-only VFS v1 backed by ps5rt::RandomAccessReader. The core may request a
// newer interface first; callers must advertise this only when the requested
// version is <= 1.
[[nodiscard]] lr::VfsInterface* vfs_interface_v1() noexcept;

} // namespace corehost
