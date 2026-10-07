#include <corehost/linked_core.hpp>
#include <corehost/ps5_runner.hpp>

int main() {
  return corehost::run_linked_core_ps5(
      {"NATIVE Odyssey2 - O2EM", "ODYSSEY2", "game.bin"},
      corehost::linked_core_api());
}
