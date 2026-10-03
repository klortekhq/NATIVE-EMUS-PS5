#include <ps5rt/input.hpp>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {
std::array<std::int32_t, 4> users{20, 10, -1, -1};
std::array<std::array<std::uint8_t, 1024>, 4> states{};
std::vector<std::int32_t> opened;
std::vector<std::int32_t> closed;
std::array<int, 3> last_rumble{-1, -1, -1};
std::int32_t borrowed_user = -1;
std::int32_t pad_init_result = 0;

std::array<std::uint8_t, 1024>& state_for_handle(std::int32_t handle) {
  const auto user = handle - 100;
  for (std::size_t i = 0; i < users.size(); ++i)
    if (users[i] == user) return states[i];
  return states[0];
}

void set_connected(std::array<std::uint8_t, 1024>& s, std::uint32_t buttons,
                   std::uint8_t lx = 128, std::uint8_t ly = 128) {
  s.fill(0);
  std::memcpy(s.data(), &buttons, sizeof(buttons));
  s[4] = lx; s[5] = ly; s[6] = 128; s[7] = 128;
  s[8] = 64; s[9] = 255;
  s[76] = 1;
}
}

extern "C" std::int32_t sceUserServiceInitialize(const void*) { return 0; }
extern "C" std::int32_t sceUserServiceGetInitialUser(std::int32_t* out) { *out = 10; return 0; }
extern "C" std::int32_t sceUserServiceGetLoginUserIdList(std::int32_t* out) {
  std::memcpy(out, users.data(), sizeof(users));
  return 0;
}
extern "C" std::int32_t scePadInit() { return pad_init_result; }
extern "C" std::int32_t scePadOpen(std::int32_t user, std::int32_t type,
                                   std::int32_t index, const void* params) {
  assert(type == 0 && index == 0 && params == nullptr);
  opened.push_back(user);
  if (user == borrowed_user) return static_cast<std::int32_t>(0x80920004u);
  return user + 100;
}
extern "C" std::int32_t scePadGetHandle(std::int32_t user, std::int32_t, std::int32_t) {
  return user + 100;
}
extern "C" std::int32_t scePadClose(std::int32_t handle) {
  closed.push_back(handle);
  return 0;
}
extern "C" std::int32_t scePadReadState(std::int32_t handle, void* out) {
  auto& state = state_for_handle(handle);
  std::memcpy(out, state.data(), state.size());
  return 0;
}
extern "C" std::int32_t scePadSetVibration(std::int32_t handle, const void* ptr) {
  const auto* bytes = static_cast<const std::uint8_t*>(ptr);
  last_rumble = {handle, bytes[0], bytes[1]};
  return 0;
}

int main() {
  // Loader environments may report ScePad already initialized. A failed
  // scePadInit must not reject working pad handles.
  pad_init_result = static_cast<std::int32_t>(0x80920002u);

  set_connected(states[0], 0x004000u, 255, 0); // user 20
  set_connected(states[1], 0x001001u);         // initial user 10: triangle + Create

  assert(ps5rt::initialize_input());

  ps5rt::InputSnapshot snapshot{};
  assert(ps5rt::poll_input(snapshot));
  assert(snapshot.controllers[0].connected);
  assert(snapshot.controllers[0].user_id == 10);
  assert(
      snapshot.controllers[0].buttons &
      static_cast<std::uint32_t>(ps5rt::Button::create));
  assert(snapshot.controllers[1].connected);
  assert(snapshot.controllers[1].user_id == 20);
  assert(ps5rt::connected_controller_count() == 2);

  assert(ps5rt::set_rumble(0, 0.5f, 1.0f));
  assert(last_rumble[0] == 110);
  assert(last_rumble[1] >= 127 && last_rumble[1] <= 128);
  assert(last_rumble[2] == 255);

  // Intercepted input must be neutralized and rumble stopped.
  std::uint32_t intercepted = 0x80000000u | 0x004000u;
  std::memcpy(states[1].data(), &intercepted, sizeof(intercepted));
  assert(ps5rt::poll_input(snapshot));
  assert(!snapshot.controllers[0].connected);
  assert(ps5rt::connected_controller_count() == 1);
  assert(last_rumble[1] == 0 && last_rumble[2] == 0);

  // User churn: remove 10, keep 20, add a pad whose handle already exists.
  users = {20, 30, -1, -1};
  borrowed_user = 30;
  set_connected(states[1], 0x004000u);
  set_connected(states[2], 0x002000u);
  assert(ps5rt::poll_input(snapshot));
  assert(snapshot.controllers[0].connected && snapshot.controllers[0].user_id == 30);
  assert(snapshot.controllers[1].connected && snapshot.controllers[1].user_id == 20);

  const auto closed_before = closed.size();
  ps5rt::shutdown_input();
  assert(ps5rt::connected_controller_count() == 0);

  // Owned user 20 closes; borrowed user 30 does not.
  assert(closed.size() == closed_before + 1);
  assert(closed.back() == 120);
  return 0;
}
