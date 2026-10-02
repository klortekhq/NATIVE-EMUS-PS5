#include <ps5rt/thread.hpp>

#include <cassert>
#include <cstddef>
#include <iostream>

int main() {
  constexpr std::size_t mib =
      1024u * 1024u;

  assert(
      ps5rt::recommended_stack_size(
          "frontend",
          384u * 1024u) ==
      384u * 1024u);

  assert(
      ps5rt::recommended_stack_size(
          "jit",
          0) ==
      2u * mib);

  assert(
      ps5rt::recommended_stack_size(
          "cpu-recompiler",
          512u * 1024u) ==
      2u * mib);

  assert(
      ps5rt::recommended_stack_size(
          "emulation-thread",
          4u * mib) ==
      4u * mib);

  std::cout
      << "ps5rt thread policy tests passed\n";
  return 0;
}
