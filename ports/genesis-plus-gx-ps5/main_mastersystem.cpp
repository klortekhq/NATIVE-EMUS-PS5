#include <corehost/linked_core.hpp>
#include <corehost/ps5_runner.hpp>

int main() {
  return corehost::run_linked_core_ps5(
      {"NATIVE Master System - Genesis Plus GX", "MASTERSYSTEM", "game.sms"},
      corehost::linked_core_api());
}
