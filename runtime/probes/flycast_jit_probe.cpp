#include <ps5rt/jit.hpp>

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace {

struct ProbeSpec {
  const char* name;
  std::size_t size;
};

bool run_probe(const ProbeSpec& spec) {
  ps5rt::JitRegion region{};
  ps5rt::JitRequest request{};
  request.size = spec.size;
  request.alignment = 0x4000;
  request.debug_name = spec.name;
  request.prefer_dual_mapping = true;

  const auto created = ps5rt::create_jit_region(request, region);
  if (!created) {
    std::printf("[flycast-jit] %s create failed code=%d native=%d %.*s\n",
                spec.name,
                static_cast<int>(created.code),
                created.native_code,
                static_cast<int>(created.message.size()),
                created.message.data());
    return false;
  }

  if (!region.write_view.address || !region.execute_view.address ||
      region.write_view.address == region.execute_view.address ||
      region.write_view.size < spec.size ||
      region.execute_view.size < spec.size) {
    std::printf("[flycast-jit] %s invalid RW/RX aliases\n", spec.name);
    (void)ps5rt::destroy_jit_region(region);
    return false;
  }

  static constexpr unsigned char code[] = {
      0xB8, 0x2A, 0x00, 0x00, 0x00, 0xC3
  };
  std::memcpy(region.write_view.address, code, sizeof(code));
  if (!ps5rt::flush_instruction_cache(region.execute_view.address, sizeof(code))) {
    std::printf("[flycast-jit] %s flush failed\n", spec.name);
    (void)ps5rt::destroy_jit_region(region);
    return false;
  }

  using ProbeFn = int (*)();
  auto fn = reinterpret_cast<ProbeFn>(region.execute_view.address);
  const int value = fn();
  const auto destroyed = ps5rt::destroy_jit_region(region);
  if (!destroyed || value != 42) {
    std::printf("[flycast-jit] %s execute/destroy failed result=%d\n",
                spec.name, value);
    return false;
  }

  std::printf("[flycast-jit] %s PASS bytes=%zu\n", spec.name, spec.size);
  return true;
}

} // namespace

int main() {
  // Exact upstream Flycast x64 JIT capacities:
  // SH4: 10 MiB main + 1 MiB temporary
  // AICA ARM7: 4 MiB
  // AICA DSP: 32 KiB
  static constexpr std::array<ProbeSpec, 3> probes{{
      {"flycast-sh4-rec-x64", 11u * 1024u * 1024u},
      {"flycast-aica-arm7-x64", 4u * 1024u * 1024u},
      {"flycast-aica-dsp-x64", 32u * 1024u},
  }};

  for (const auto& probe : probes) {
    if (!run_probe(probe))
      return 1;
  }
  return 0;
}
