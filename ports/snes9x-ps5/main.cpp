#include "../common/libretro-static/linked_core.hpp"

int main() {
  const native_emu::libretro::PortConfig config{
      "NATIVE SNES - Snes9x",
      "SNES",
      "game.sfc",
  };
  return native_emu::libretro::run_linked_port(config);
}
