#include "../common/libretro-static/linked_core.hpp"
int main() {
  return native_emu::libretro::run_linked_port({
      "NATIVE Game Gear - Genesis Plus GX", "GAMEGEAR", "game.gg"});
}
