#include <ps5rt/input.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

extern "C" {
std::int32_t sceUserServiceInitialize(const void* params);
std::int32_t sceUserServiceGetInitialUser(std::int32_t* user_id);
std::int32_t sceUserServiceGetLoginUserIdList(std::int32_t* user_ids);

std::int32_t scePadInit();
std::int32_t scePadOpen(std::int32_t user_id, std::int32_t port_type,
                        std::int32_t index, const void* params);
std::int32_t scePadGetHandle(std::int32_t user_id, std::int32_t port_type,
                             std::int32_t index);
std::int32_t scePadClose(std::int32_t handle);
std::int32_t scePadReadState(std::int32_t handle, void* state);
std::int32_t scePadSetVibration(std::int32_t handle, const void* params);
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

constexpr std::uint32_t kPadAlreadyOpened = 0x80920004u;
constexpr std::size_t kPadStateBytes = 1024;
constexpr std::size_t kConnectedOffset = 76;
constexpr std::size_t kTimestampOffset = 80;
constexpr std::size_t kMinimumStateBytes = 88;

struct Slot {
  std::int32_t user{-1};
  std::int32_t handle{-1};
  bool owned{};
  bool signed_in{};
  bool vibrating{};
};

std::array<Slot, InputSnapshot::max_controllers> g_slots{};
std::int32_t g_initial_user{-1};
bool g_initialized{};

float axis(std::uint8_t raw) noexcept {
  const int delta = static_cast<int>(raw) - 128;
  const int denominator = delta < 0 ? 128 : 127;
  return std::clamp(static_cast<float>(delta) / static_cast<float>(denominator),
                    -1.0f, 1.0f);
}

std::uint32_t translate_buttons(std::uint32_t raw) noexcept {
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

void stop_vibration(Slot& slot) noexcept {
  if (!slot.vibrating || slot.handle < 0) return;
  const std::array<std::uint8_t, 2> zero{};
  if (scePadSetVibration(slot.handle, zero.data()) >= 0)
    slot.vibrating = false;
}

void close_slot(Slot& slot) noexcept {
  stop_vibration(slot);
  if (slot.owned && slot.handle >= 0)
    (void)scePadClose(slot.handle);
  slot = {};
}

bool contains_user(const std::array<std::int32_t, 4>& users,
                   std::int32_t user) noexcept {
  return std::find(users.begin(), users.end(), user) != users.end();
}

Result refresh_slots() noexcept {
  std::array<std::int32_t, 4> users{-1, -1, -1, -1};
  const auto rc = sceUserServiceGetLoginUserIdList(users.data());
  if (rc < 0) {
    for (auto& slot : g_slots) {
      stop_vibration(slot);
      slot.signed_in = false;
    }
    return {ErrorCode::system_error, rc, "sceUserServiceGetLoginUserIdList failed"};
  }

  const auto first = std::find(users.begin(), users.end(), g_initial_user);
  if (first != users.end() && first != users.begin())
    std::iter_swap(users.begin(), first);

  for (auto& slot : g_slots) {
    if (slot.handle < 0) continue;
    slot.signed_in = contains_user(users, slot.user);
    if (!slot.signed_in)
      close_slot(slot);
  }

  for (const auto user : users) {
    if (user < 0) continue;

    const bool already_assigned = std::any_of(
        g_slots.begin(), g_slots.end(),
        [user](const Slot& slot) { return slot.handle >= 0 && slot.user == user; });
    if (already_assigned) continue;

    auto free_slot = std::find_if(
        g_slots.begin(), g_slots.end(),
        [](const Slot& slot) { return slot.handle < 0; });
    if (free_slot == g_slots.end()) break;

    std::int32_t handle = scePadOpen(user, 0, 0, nullptr);
    bool owned = true;
    if (static_cast<std::uint32_t>(handle) == kPadAlreadyOpened) {
      handle = scePadGetHandle(user, 0, 0);
      owned = false;
    }
    if (handle < 0) continue;

    free_slot->user = user;
    free_slot->handle = handle;
    free_slot->owned = owned;
    free_slot->signed_in = true;
  }

  return Result::success();
}

bool decode_state(const std::array<std::uint8_t, kPadStateBytes>& bytes,
                  ControllerState& out) noexcept {
  static_assert(kMinimumStateBytes <= kPadStateBytes);

  std::uint32_t raw_buttons = 0;
  std::memcpy(&raw_buttons, bytes.data(), sizeof(raw_buttons));

  if (bytes[kConnectedOffset] == 0 || (raw_buttons & kIntercepted) != 0)
    return false;

  out.connected = true;
  out.buttons = translate_buttons(raw_buttons);
  out.left = {axis(bytes[4]), axis(bytes[5])};
  out.right = {axis(bytes[6]), axis(bytes[7])};
  out.l2 = static_cast<float>(bytes[8]) / 255.0f;
  out.r2 = static_cast<float>(bytes[9]) / 255.0f;
  return true;
}

std::uint8_t rumble_byte(float value) noexcept {
  value = std::clamp(value, 0.0f, 1.0f);
  return static_cast<std::uint8_t>(std::lround(value * 255.0f));
}

} // namespace

Result initialize_input() noexcept {
  shutdown_input();

  // UserService is process-wide. Initialization is intentionally idempotent;
  // another PS5 subsystem may already own it.
  (void)sceUserServiceInitialize(nullptr);

  auto rc = sceUserServiceGetInitialUser(&g_initial_user);
  if (rc < 0)
    return {ErrorCode::system_error, rc, "sceUserServiceGetInitialUser failed"};

  rc = scePadInit();
  if (rc < 0)
    return {ErrorCode::system_error, rc, "scePadInit failed"};

  g_initialized = true;
  const auto refreshed = refresh_slots();
  if (!refreshed) {
    shutdown_input();
    return refreshed;
  }
  return Result::success();
}

Result poll_input(InputSnapshot& out) noexcept {
  out = {};
  if (!g_initialized)
    return {ErrorCode::system_error, 0, "input not initialized"};

  const auto refreshed = refresh_slots();
  if (!refreshed)
    return refreshed;

  for (std::size_t i = 0; i < g_slots.size(); ++i) {
    auto& slot = g_slots[i];
    if (slot.handle < 0 || !slot.signed_in) continue;

    alignas(16) std::array<std::uint8_t, kPadStateBytes> bytes{};
    const auto rc = scePadReadState(slot.handle, bytes.data());
    if (rc < 0) {
      stop_vibration(slot);
      continue;
    }

    auto& controller = out.controllers[i];
    controller.user_id = static_cast<std::uint32_t>(slot.user);
    if (!decode_state(bytes, controller)) {
      stop_vibration(slot);
      controller = {};
    }
  }

  return Result::success();
}

Result set_rumble(std::size_t controller, float low, float high) noexcept {
  if (!g_initialized || controller >= g_slots.size())
    return {ErrorCode::invalid_argument, 0, "invalid controller"};

  auto& slot = g_slots[controller];
  if (slot.handle < 0 || !slot.signed_in)
    return {ErrorCode::system_error, 0, "controller is not connected"};

  const std::array<std::uint8_t, 2> params{
      rumble_byte(low), rumble_byte(high)};
  const auto rc = scePadSetVibration(slot.handle, params.data());
  if (rc < 0)
    return {ErrorCode::system_error, rc, "scePadSetVibration failed"};

  slot.vibrating = params[0] != 0 || params[1] != 0;
  return Result::success();
}

void shutdown_input() noexcept {
  for (auto& slot : g_slots)
    close_slot(slot);
  g_initial_user = -1;
  g_initialized = false;

  // Do not terminate UserService here. It is process-wide and may be shared
  // with VideoOut, account-aware storage, or another emulator subsystem.
}

} // namespace ps5rt
