#include <corehost/linked_core.hpp>
#include <corehost/ps5_runner.hpp>

int main() {
  return corehost::run_linked_core_ps5(
      {"NATIVE Mega Drive - Genesis Plus GX", "MEGADRIVE", "game.md"},
      corehost::linked_core_api());
}
