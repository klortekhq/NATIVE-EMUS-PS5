#include <corehost/linked_core.hpp>
#include <corehost/ps5_runner.hpp>

int main() {
  return corehost::run_linked_core_ps5(
      {"NATIVE Game Boy Color - Gambatte", "GBC", "game.gbc"},
      corehost::linked_core_api());
}
