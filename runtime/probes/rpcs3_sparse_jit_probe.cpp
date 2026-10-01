#include <ps5rt/c/sparse_arena.h>
#include <ps5rt/jit.hpp>
#include <ps5rt/memory.hpp>

#include <cstddef>
#include <cstdio>
#include <cstring>

namespace {

constexpr std::size_t kArenaSize = 0x80000000ull;   // 2 GiB
constexpr std::size_t kCodeCommit = 0x80000ull;     // 512 KiB
constexpr std::size_t kDataOffset = 0x40000000ull;  // 1 GiB
constexpr std::size_t kDataCommit = 0x200000ull;    // 2 MiB

bool write_and_execute(ps5rt_sparse_arena& arena, unsigned value) {
  static constexpr unsigned char prefix[] = {0xB8};
  static constexpr unsigned char suffix[] = {0xC3};

  auto* code = static_cast<unsigned char*>(arena.base);
  code[0] = prefix[0];
  std::memcpy(code + 1, &value, sizeof(value));
  code[5] = suffix[0];

  if (!ps5rt::flush_instruction_cache(code, 6)) {
    std::printf("[rpcs3-sparse] cache flush failed\n");
    return false;
  }

  using ProbeFn = int (*)();
  auto fn = reinterpret_cast<ProbeFn>(code);
  const int got = fn();
  if (got != static_cast<int>(value)) {
    std::printf("[rpcs3-sparse] execute mismatch got=%d expected=%u\n", got, value);
    return false;
  }
  return true;
}

} // namespace

int main() {
  std::size_t flexible_before = 0;
  std::size_t flexible_after = 0;
  const auto before_result =
      ps5rt::query_available_memory(ps5rt::MemoryKind::flexible, flexible_before);

  ps5rt_sparse_arena arena{};
  int rc = ps5rt_sparse_arena_create(
      kArenaSize, nullptr, 0x200000, &arena);
  if (rc != 0 || !arena.base || arena.size != kArenaSize) {
    std::printf("[rpcs3-sparse] reserve failed rc=%d base=%p size=%zu\n",
                rc, arena.base, arena.size);
    return 1;
  }
  std::printf("[rpcs3-sparse] reserve PASS base=%p size=%zu\n",
              arena.base, arena.size);

  rc = ps5rt_sparse_arena_commit(
      &arena, 0, kCodeCommit,
      PS5RT_SPARSE_READ | PS5RT_SPARSE_WRITE | PS5RT_SPARSE_EXEC);
  if (rc != 0) {
    std::printf("[rpcs3-sparse] 512KiB code commit failed rc=%d\n", rc);
    ps5rt_sparse_arena_destroy(&arena);
    return 2;
  }

  rc = ps5rt_sparse_arena_commit(
      &arena, kDataOffset, kDataCommit,
      PS5RT_SPARSE_READ | PS5RT_SPARSE_WRITE);
  if (rc != 0) {
    std::printf("[rpcs3-sparse] 2MiB data commit failed rc=%d\n", rc);
    ps5rt_sparse_arena_destroy(&arena);
    return 3;
  }

  if (arena.committed != kCodeCommit + kDataCommit) {
    std::printf("[rpcs3-sparse] committed accounting mismatch=%zu\n",
                arena.committed);
    ps5rt_sparse_arena_destroy(&arena);
    return 4;
  }

  auto* data = static_cast<unsigned char*>(arena.base) + kDataOffset;
  data[0] = 0x5A;
  data[kDataCommit - 1] = 0xA5;
  if (data[0] != 0x5A || data[kDataCommit - 1] != 0xA5) {
    std::printf("[rpcs3-sparse] data mapping check failed\n");
    ps5rt_sparse_arena_destroy(&arena);
    return 5;
  }

  if (!write_and_execute(arena, 42)) {
    ps5rt_sparse_arena_destroy(&arena);
    return 6;
  }

  rc = ps5rt_sparse_arena_protect(
      &arena, 0, kCodeCommit, PS5RT_SPARSE_READ | PS5RT_SPARSE_EXEC);
  if (rc != 0) {
    std::printf("[rpcs3-sparse] RX protect failed rc=%d\n", rc);
    ps5rt_sparse_arena_destroy(&arena);
    return 7;
  }

  void* const stable_base = arena.base;
  rc = ps5rt_sparse_arena_decommit(&arena, 0, kCodeCommit);
  if (rc != 0 || arena.base != stable_base || arena.committed != kDataCommit) {
    std::printf("[rpcs3-sparse] code decommit failed rc=%d base=%p committed=%zu\n",
                rc, arena.base, arena.committed);
    ps5rt_sparse_arena_destroy(&arena);
    return 8;
  }

  rc = ps5rt_sparse_arena_commit(
      &arena, 0, kCodeCommit,
      PS5RT_SPARSE_READ | PS5RT_SPARSE_WRITE | PS5RT_SPARSE_EXEC);
  if (rc != 0 || arena.base != stable_base) {
    std::printf("[rpcs3-sparse] code recommit failed rc=%d base=%p\n",
                rc, arena.base);
    ps5rt_sparse_arena_destroy(&arena);
    return 9;
  }

  if (!write_and_execute(arena, 43)) {
    ps5rt_sparse_arena_destroy(&arena);
    return 10;
  }

  const auto after_result =
      ps5rt::query_available_memory(ps5rt::MemoryKind::flexible, flexible_after);

  std::printf("[rpcs3-sparse] committed_direct=%zu\n", arena.committed);
  if (before_result && after_result) {
    std::printf("[rpcs3-sparse] flexible_before=%zu flexible_after=%zu delta=%lld\n",
                flexible_before, flexible_after,
                static_cast<long long>(flexible_after) -
                    static_cast<long long>(flexible_before));
  } else {
    std::printf("[rpcs3-sparse] flexible-memory accounting unavailable\n");
  }

  ps5rt_sparse_arena_destroy(&arena);
  if (arena.base || arena.size || arena.committed) {
    std::printf("[rpcs3-sparse] destroy did not clear arena\n");
    return 11;
  }

  std::printf("[rpcs3-sparse] PASS\n");
  return 0;
}
