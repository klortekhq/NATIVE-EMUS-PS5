#include <corehost/linked_core.hpp>
#include <corehost/ps5_runner.hpp>

int main() {
  return corehost::run_linked_core_ps5(
      {"NATIVE Game Gear - Genesis Plus GX", "GAMEGEAR", "game.gg"},
      corehost::linked_core_api());
}
