#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ps5rt_sparse_arena {
  void* base;
  size_t size;
  size_t committed;
} ps5rt_sparse_arena;

enum {
  PS5RT_SPARSE_READ  = 1u << 0,
  PS5RT_SPARSE_WRITE = 1u << 1,
  PS5RT_SPARSE_EXEC  = 1u << 2,
};

// Reserve a large virtual-address arena without backing it with physical
// memory. The returned base remains stable for the arena lifetime.
int ps5rt_sparse_arena_create(
    size_t size, void* hint, size_t alignment, ps5rt_sparse_arena* out);

// Commit a direct-memory-backed chunk at base + offset. Offset and size are
// page-granular. Executable chunks remain at the same virtual address; the
// backend maps RW first and then promotes protection when required.
int ps5rt_sparse_arena_commit(
    ps5rt_sparse_arena* arena, size_t offset, size_t size, unsigned protection);

// Release every fully-covered committed chunk in [offset, offset + size) and
// restore virtual reservations at the same addresses. Partial-chunk decommit
// requests are rejected.
int ps5rt_sparse_arena_decommit(
    ps5rt_sparse_arena* arena, size_t offset, size_t size);

// Change protection on an already committed range.
int ps5rt_sparse_arena_protect(
    ps5rt_sparse_arena* arena, size_t offset, size_t size, unsigned protection);

// Release committed chunks and the enclosing virtual reservation.
void ps5rt_sparse_arena_destroy(ps5rt_sparse_arena* arena);

#ifdef __cplusplus
}
#endif
