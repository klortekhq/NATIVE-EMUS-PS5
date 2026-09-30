#include "../common/libretro-static/linked_core.hpp"

int main() {
  const native_emu::libretro::PortConfig config{
      "NATIVE NES - FCEUmm",
      "NES",
      "game.nes",
  };
  return native_emu::libretro::run_linked_port(config);
}
