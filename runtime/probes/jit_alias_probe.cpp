#include <ps5rt/jit.hpp>

#include <cstdint>
#include <cstdio>
#include <cstring>

int main() {
  ps5rt::JitRegion region{};
  ps5rt::JitRequest request{};
  request.size = 0x4000;
  request.alignment = 0x4000;
  request.debug_name = "jit-alias-probe";
  request.prefer_dual_mapping = true;

  const auto created = ps5rt::create_jit_region(request, region);
  if (!created) {
    std::printf("[jit-probe] create failed code=%d native=%d %.*s\n",
                static_cast<int>(created.code), created.native_code,
                static_cast<int>(created.message.size()), created.message.data());
    return 1;
  }

  std::printf("[jit-probe] RW=%p RX=%p bytes=%zu\n",
              region.write_view.address, region.execute_view.address,
              region.execute_view.size);

  if (!region.write_view.address || !region.execute_view.address ||
      region.write_view.address == region.execute_view.address) {
    std::puts("[jit-probe] expected distinct RW/RX aliases");
    (void)ps5rt::destroy_jit_region(region);
    return 2;
  }

  // x86-64: mov eax, 42 ; ret
  static constexpr std::uint8_t code[] = {
      0xB8, 0x2A, 0x00, 0x00, 0x00, 0xC3
  };
  std::memcpy(region.write_view.address, code, sizeof(code));

  const auto flushed = ps5rt::flush_instruction_cache(
      region.execute_view.address, sizeof(code));
  if (!flushed) {
    std::puts("[jit-probe] instruction cache flush failed");
    (void)ps5rt::destroy_jit_region(region);
    return 3;
  }

  using ProbeFn = int (*)();
  auto fn = reinterpret_cast<ProbeFn>(region.execute_view.address);
  const int value = fn();

  std::printf("[jit-probe] executed result=%d\n", value);
  const auto destroyed = ps5rt::destroy_jit_region(region);
  if (!destroyed) {
    std::printf("[jit-probe] destroy failed native=%d\n", destroyed.native_code);
    return 4;
  }

  if (value != 42) {
    std::puts("[jit-probe] FAIL: RX alias did not execute emitted code");
    return 5;
  }

  std::puts("[jit-probe] PASS: RW emission -> RX execution");
  return 0;
}
