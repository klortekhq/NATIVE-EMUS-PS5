#include <corehost/linked_core.hpp>
#include <corehost/ps5_runner.hpp>

int main() {
  return corehost::run_linked_core_ps5(
      {"NATIVE SNES - Snes9x", "SNES", "game.sfc"},
      corehost::linked_core_api());
}
