#pragma once

#include <ps5rt/result.hpp>

namespace ps5rt {

// Registers the read-only smb:// RandomAccessReader backend.
//
// Credentials are loaded by the PS5 implementation from:
//   /data/NATIVE-EMUS-PS5/network/smb.ini
//
// Usernames/passwords are never accepted in content URIs.
Result initialize_smb_backend() noexcept;
void shutdown_smb_backend() noexcept;

} // namespace ps5rt
