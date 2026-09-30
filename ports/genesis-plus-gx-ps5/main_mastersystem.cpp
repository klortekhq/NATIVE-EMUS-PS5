#include "../common/libretro-static/linked_core.hpp"
int main() {
  return native_emu::libretro::run_linked_port({
      "NATIVE Master System - Genesis Plus GX", "MASTERSYSTEM", "game.sms"});
}
