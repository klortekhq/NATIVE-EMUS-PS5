#include <ps5rt/input.hpp>

#include <cstddef>
#include <cstdint>

extern "C" {
std::int32_t scePadInit();
std::int32_t scePadOpen(std::int32_t user_id, std::int32_t port_type, std::int32_t index,
                        const void* params);
std::int32_t scePadRead(std::int32_t handle, void* samples, std::int32_t capacity);
std::int32_t scePadClose(std::int32_t handle);
std::int32_t sceUserServiceInitialize(const void* params);
std::int32_t sceUserServiceGetInitialUser(std::int32_t* user_id);
std::int32_t sceUserServiceTerminate();
std::int32_t sceKernelUsleep(std::uint32_t microseconds);
}

namespace ps5rt {
namespace {

constexpr std::uint32_t kL3 = 0x000002u;
constexpr std::uint32_t kR3 = 0x000004u;
constexpr std::uint32_t kOptions = 0x000008u;
constexpr std::uint32_t kUp = 0x000010u;
constexpr std::uint32_t kRight = 0x000020u;
constexpr std::uint32_t kDown = 0x000040u;
constexpr std::uint32_t kLeft = 0x000080u;
constexpr std::uint32_t kL1 = 0x000400u;
constexpr std::uint32_t kR1 = 0x000800u;
constexpr std::uint32_t kTriangle = 0x001000u;
constexpr std::uint32_t kCircle = 0x002000u;
constexpr std::uint32_t kCross = 0x004000u;
constexpr std::uint32_t kSquare = 0x008000u;
constexpr std::uint32_t kTouchpad = 0x100000u;
constexpr std::uint32_t kIntercepted = 0x80000000u;

struct PadSample {
  std::uint32_t buttons;
  std::uint8_t left_x;
  std::uint8_t left_y;
  std::uint8_t right_x;
  std::uint8_t right_y;
  std::uint8_t left_trigger;
  std::uint8_t right_trigger;
  std::uint8_t reserved_to_connected[66];
  std::int32_t connected;
  std::uint64_t timestamp_us;
  std::uint8_t extension[16];
  std::uint8_t connected_count;
  std::uint8_t remaining[15];
};

static_assert(sizeof(PadSample) == 120);
static_assert(offsetof(PadSample, connected) == 0x4c);
static_assert(offsetof(PadSample, timestamp_us) == 0x50);

std::int32_t g_handle = -1;
bool g_owns_user_service = false;

float axis(std::uint8_t v) noexcept {
  const int offset = static_cast<int>(v) - 128;
  const int denom = offset < 0 ? 128 : 127;
  float result = static_cast<float>(offset) / static_cast<float>(denom);
  if (result < -1.0f) result = -1.0f;
  if (result > 1.0f) result = 1.0f;
  return result;
}

std::uint32_t buttons(std::uint32_t raw) noexcept {
  std::uint32_t out = 0;
  if (raw & kUp) out |= static_cast<std::uint32_t>(Button::up);
  if (raw & kDown) out |= static_cast<std::uint32_t>(Button::down);
  if (raw & kLeft) out |= static_cast<std::uint32_t>(Button::left);
  if (raw & kRight) out |= static_cast<std::uint32_t>(Button::right);
  if (raw & kCross) out |= static_cast<std::uint32_t>(Button::cross);
  if (raw & kCircle) out |= static_cast<std::uint32_t>(Button::circle);
  if (raw & kSquare) out |= static_cast<std::uint32_t>(Button::square);
  if (raw & kTriangle) out |= static_cast<std::uint32_t>(Button::triangle);
  if (raw & kL1) out |= static_cast<std::uint32_t>(Button::l1);
  if (raw & kR1) out |= static_cast<std::uint32_t>(Button::r1);
  if (raw & kL3) out |= static_cast<std::uint32_t>(Button::l3);
  if (raw & kR3) out |= static_cast<std::uint32_t>(Button::r3);
  if (raw & kOptions) out |= static_cast<std::uint32_t>(Button::options);
  if (raw & kTouchpad) out |= static_cast<std::uint32_t>(Button::touchpad);
  return out;
}

} // namespace

Result initialize_input() noexcept {
  shutdown_input();
  g_owns_user_service = (sceUserServiceInitialize(nullptr) == 0);

  std::int32_t user = -1;
  std::int32_t rc = sceUserServiceGetInitialUser(&user);
  if (rc < 0)
    return {ErrorCode::system_error, rc, "sceUserServiceGetInitialUser failed"};
  rc = scePadInit();
  if (rc < 0)
    return {ErrorCode::system_error, rc, "scePadInit failed"};

  for (int attempt = 0; attempt < 10; ++attempt) {
    g_handle = scePadOpen(user, 0, 0, nullptr);
    if (g_handle >= 0)
      return Result::success();
    (void)sceKernelUsleep(100000);
  }
  return {ErrorCode::system_error, g_handle, "scePadOpen failed"};
}

Result poll_input(InputSnapshot& out) noexcept {
  out = {};
  if (g_handle < 0)
    return {ErrorCode::system_error, g_handle, "input not initialized"};

  PadSample samples[64]{};
  const std::int32_t count = scePadRead(g_handle, samples, 64);
  if (count <= 0)
    return Result::success();
  if (count > 64)
    return {ErrorCode::system_error, count, "scePadRead returned invalid count"};

  const PadSample* newest = nullptr;
  for (std::int32_t i = 0; i < count; ++i) {
    if (!newest || samples[i].timestamp_us > newest->timestamp_us)
      newest = &samples[i];
  }
  if (!newest || !newest->connected || (newest->buttons & kIntercepted))
    return Result::success();

  auto& pad = out.controllers[0];
  pad.connected = true;
  pad.buttons = buttons(newest->buttons);
  pad.left = {axis(newest->left_x), axis(newest->left_y)};
  pad.right = {axis(newest->right_x), axis(newest->right_y)};
  pad.l2 = static_cast<float>(newest->left_trigger) / 255.0f;
  pad.r2 = static_cast<float>(newest->right_trigger) / 255.0f;
  return Result::success();
}

Result set_rumble(std::size_t, float, float) noexcept {
  return {ErrorCode::unsupported, 0, "rumble backend not yet migrated"};
}

void shutdown_input() noexcept {
  if (g_handle >= 0) {
    (void)scePadClose(g_handle);
    g_handle = -1;
  }
  if (g_owns_user_service) {
    (void)sceUserServiceTerminate();
    g_owns_user_service = false;
  }
}

} // namespace ps5rt
