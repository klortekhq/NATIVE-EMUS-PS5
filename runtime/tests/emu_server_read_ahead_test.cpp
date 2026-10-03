#include "emu_server_read_ahead.hpp"

#include <cassert>
#include <cstddef>

int main() {
  using ps5rt::detail::EmuServerReadAheadPolicy;

  {
    EmuServerReadAheadPolicy disabled(0, 0);
    assert(disabled.suggest(0, 4096, 65536) == 4096);
    assert(disabled.suggest(4096, 4096, 61440) == 4096);
  }

  {
    // max=0 preserves the existing fixed read-ahead behavior.
    EmuServerReadAheadPolicy fixed(16 * 1024, 0);
    assert(fixed.base_bytes() == 16 * 1024);
    assert(fixed.max_bytes() == 16 * 1024);
    assert(fixed.suggest(0, 4096, 128 * 1024) == 16 * 1024);
    assert(fixed.suggest(4096, 4096, 124 * 1024) == 16 * 1024);
  }

  {
    EmuServerReadAheadPolicy adaptive(16 * 1024, 64 * 1024);
    assert(adaptive.suggest(0, 4096, 256 * 1024) == 16 * 1024);
    assert(adaptive.suggest(4096, 4096, 252 * 1024) == 32 * 1024);
    assert(adaptive.suggest(8192, 4096, 248 * 1024) == 64 * 1024);
    assert(adaptive.suggest(12288, 4096, 244 * 1024) == 64 * 1024);

    // A seek breaks the sequential streak and returns to the configured base.
    assert(adaptive.suggest(128 * 1024, 4096, 128 * 1024) == 16 * 1024);

    // Large logical reads are never truncated to the read-ahead window.
    assert(adaptive.suggest(132 * 1024, 96 * 1024, 120 * 1024) == 96 * 1024);

    adaptive.reset();
    assert(adaptive.suggest(0, 4096, 8192) == 8192);
  }

  {
    // A max below the base is normalized to fixed-base behavior.
    EmuServerReadAheadPolicy normalized(32 * 1024, 4096);
    assert(normalized.max_bytes() == 32 * 1024);
  }

  return 0;
}
