#include "../common/libretro-static/linked_core.hpp"
int main() {
  return native_emu::libretro::run_linked_port({
      "NATIVE SG-1000 - Genesis Plus GX", "SG1000", "game.sg"});
}
