#include <ps5rt/app.hpp>

#include <cstdint>

extern "C" {
std::int32_t sceUserServiceInitialize(const void* params);
std::int32_t sceUserServiceGetInitialUser(std::int32_t* user_id);
std::int32_t sceUserServiceTerminate();
}

namespace ps5rt {
namespace {
bool g_user_service = false;
}

Result initialize_app(const AppConfig& config, AppInfo& out) noexcept {
  out = {};
  if (!config.initialize_user_service)
    return Result::success();

  const std::int32_t init = sceUserServiceInitialize(nullptr);
  // Some jailbreak environments may have initialized UserService already.
  // The reliable gate is whether GetInitialUser succeeds.
  std::int32_t user = -1;
  const std::int32_t rc = sceUserServiceGetInitialUser(&user);
  if (rc < 0)
    return {ErrorCode::system_error, rc, "sceUserServiceGetInitialUser failed"};

  g_user_service = (init == 0);
  out.active_user_id = static_cast<std::uint32_t>(user);
  out.network_available = false;
  return Result::success();
}

void shutdown_app() noexcept {
  if (g_user_service) {
    (void)sceUserServiceTerminate();
    g_user_service = false;
  }
}

} // namespace ps5rt
