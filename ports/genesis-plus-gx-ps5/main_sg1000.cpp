#include <corehost/linked_core.hpp>
#include <corehost/ps5_runner.hpp>

int main() {
  return corehost::run_linked_core_ps5(
      {"NATIVE SG-1000 - Genesis Plus GX", "SG1000", "game.sg"},
      corehost::linked_core_api());
}
