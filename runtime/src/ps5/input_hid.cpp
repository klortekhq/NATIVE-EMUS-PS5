#include <ps5rt/input.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <sys/stat.h>

extern "C" {
int sceKernelLoadStartModule(const char* path, size_t args, const void* argp,
                             std::uint32_t flags, void* opt, int* res);
int sceKernelDlsym(int handle, const char* symbol, void** address);
}

namespace ps5rt::detail {
namespace {

using KeyboardInit = int (*)();
using KeyboardOpen = int (*)(std::int32_t, std::int32_t, std::int32_t, const void*);
using KeyboardRead = int (*)(std::int32_t, void*);
using KeyboardClose = int (*)(std::int32_t);
using MouseInit = int (*)();
using MouseOpen = int (*)(std::int32_t, std::int32_t, std::int32_t, const void*);
using MouseRead = int (*)(std::int32_t, void*, std::int32_t);
using MouseClose = int (*)(std::int32_t);

struct HidApi {
  KeyboardRead keyboard_read{};
  KeyboardClose keyboard_close{};
  MouseRead mouse_read{};
  MouseClose mouse_close{};
  std::int32_t keyboard{-1};
  std::int32_t mouse{-1};
};

HidApi g_hid{};

template <typename T>
bool symbol(int module, const char* name, T& out) noexcept {
  void* address = nullptr;
  if (module < 0 || sceKernelDlsym(module, name, &address) != 0 || !address)
    return false;
  out = reinterpret_cast<T>(address);
  return true;
}

int load_module(const char* name) noexcept {
  static constexpr const char* roots[] = {
      "/system/common/lib/",
      "/system/priv/lib/",
      "/system_ex/common_ex/lib/",
      "/system_ex/priv_ex/lib/",
  };

  for (const char* root : roots) {
    const std::string path = std::string(root) + name;
    struct stat st {};
    if (::stat(path.c_str(), &st) != 0)
      continue;

    int start_result = 0;
    const int module = sceKernelLoadStartModule(
        path.c_str(), 0, nullptr, 0, nullptr, &start_result);
    if (module >= 0)
      return module;
  }
  return -1;
}

void set_hid_key(KeyboardState& out, unsigned usage) noexcept {
  if (usage >= 256u)
    return;
  out.pressed[usage >> 6u] |=
      std::uint64_t{1} << (usage & 63u);
}

} // namespace

bool hid_initialize(std::int32_t user) noexcept {
  g_hid = {};

  if (const int module = load_module("libSceKeyboard.sprx"); module >= 0) {
    KeyboardInit init{};
    KeyboardOpen open{};
    if (symbol(module, "sceKeyboardInit", init) &&
        symbol(module, "sceKeyboardOpen", open) &&
        symbol(module, "sceKeyboardReadState", g_hid.keyboard_read)) {
      (void)symbol(module, "sceKeyboardClose", g_hid.keyboard_close);
      (void)init();
      std::array<std::uint8_t, 16> params{};
      g_hid.keyboard = open(user, 0, 0, params.data());
      if (g_hid.keyboard < 0)
        g_hid.keyboard_read = nullptr;
    }
  }

  if (const int module = load_module("libSceMouse.sprx"); module >= 0) {
    MouseInit init{};
    MouseOpen open{};
    if (symbol(module, "sceMouseInit", init) &&
        symbol(module, "sceMouseOpen", open) &&
        symbol(module, "sceMouseRead", g_hid.mouse_read)) {
      (void)symbol(module, "sceMouseClose", g_hid.mouse_close);
      (void)init();

      // behaviorFlag=1 requests all mice through one handle. Fall back to
      // zeroed params if the running firmware rejects that mode.
      std::array<std::uint8_t, 8> params{};
      params[0] = 1;
      g_hid.mouse = open(user, 0, 0, params.data());
      if (g_hid.mouse < 0) {
        params = {};
        g_hid.mouse = open(user, 0, 0, params.data());
      }
      if (g_hid.mouse < 0)
        g_hid.mouse_read = nullptr;
    }
  }

  // HID is optional. A console without loadable keyboard/mouse libraries must
  // still be a valid DualSense-only host.
  return true;
}

void hid_poll(InputSnapshot& out) noexcept {
  if (g_hid.keyboard >= 0 && g_hid.keyboard_read) {
    alignas(8) std::array<std::uint8_t, 128> state{};
    if (g_hid.keyboard_read(g_hid.keyboard, state.data()) == 0 &&
        state[16] != 0) {
      out.keyboard.connected = true;

      std::uint32_t modifiers = 0;
      std::memcpy(&modifiers, state.data() + 28, sizeof(modifiers));
      for (unsigned bit = 0; bit < 8; ++bit)
        if (modifiers & (1u << bit))
          set_hid_key(out.keyboard, 0xe0u + bit);

      for (unsigned i = 0; i < 16; ++i) {
        std::uint16_t usage = 0;
        std::memcpy(&usage, state.data() + 32 + i * 2, sizeof(usage));
        if (usage >= 4 && usage < 0xe0)
          set_hid_key(out.keyboard, usage);
      }
    }
  }

  if (g_hid.mouse >= 0 && g_hid.mouse_read) {
    constexpr int records_requested = 8;
    constexpr std::size_t record_bytes = 40;
    alignas(8) std::array<std::uint8_t, records_requested * 128> bytes{};

    const int count = g_hid.mouse_read(
        g_hid.mouse, bytes.data(), records_requested);
    if (count > 0 && count <= records_requested) {
      std::int64_t dx = 0;
      std::int64_t dy = 0;
      std::int64_t wheel = 0;
      std::uint32_t buttons = 0;
      bool connected = false;

      for (int i = 0; i < count; ++i) {
        const auto* record =
            bytes.data() + static_cast<std::size_t>(i) * record_bytes;
        if (record[8] == 0)
          continue;

        connected = true;
        std::int32_t x = 0, y = 0, w = 0;
        std::memcpy(&buttons, record + 12, sizeof(buttons));
        std::memcpy(&x, record + 16, sizeof(x));
        std::memcpy(&y, record + 20, sizeof(y));
        std::memcpy(&w, record + 24, sizeof(w));

        // Reject implausible values if a future firmware changes the record
        // layout rather than feeding garbage movement into an emulator.
        if (x >= -4096 && x <= 4096 && y >= -4096 && y <= 4096) {
          dx += x;
          dy += y;
        }
        if (w >= -128 && w <= 128)
          wheel += w;
      }

      if (connected) {
        out.mouse.connected = true;
        out.mouse.delta_x = static_cast<std::int32_t>(
            std::clamp<std::int64_t>(dx, INT32_MIN, INT32_MAX));
        out.mouse.delta_y = static_cast<std::int32_t>(
            std::clamp<std::int64_t>(dy, INT32_MIN, INT32_MAX));
        out.mouse.wheel_y = static_cast<std::int32_t>(
            std::clamp<std::int64_t>(wheel, INT32_MIN, INT32_MAX));
        out.mouse.buttons = buttons;
      }
    }
  }
}

void hid_shutdown() noexcept {
  if (g_hid.keyboard >= 0 && g_hid.keyboard_close)
    (void)g_hid.keyboard_close(g_hid.keyboard);
  if (g_hid.mouse >= 0 && g_hid.mouse_close)
    (void)g_hid.mouse_close(g_hid.mouse);
  g_hid = {};
}

} // namespace ps5rt::detail
