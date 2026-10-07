#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  // Request distinct RW and RX aliases backed by the same executable memory.
  PS5RT_JIT_DUAL_VIEW = 1u << 0,
};

typedef struct ps5rt_jit_region {
  void* write_view;
  void* execute_view;
  size_t size;
} ps5rt_jit_region;

// Creates executable memory suitable for native recompilers.
// Returns 0 on success. When PS5RT_JIT_DUAL_VIEW is requested, write_view and
// execute_view address the same backing storage through separate permissions.
int ps5rt_jit_create(size_t size,
                     size_t alignment,
                     unsigned flags,
                     ps5rt_jit_region* out);

// Releases all mappings/backing storage. Safe on an all-zero region.
int ps5rt_jit_destroy(ps5rt_jit_region* region);

// Synchronizes generated code before execution. Offset/size refer to the RX
// view; size==0 means the remainder of the region.
int ps5rt_jit_flush(const ps5rt_jit_region* region,
                    size_t offset,
                    size_t size);

#ifdef __cplusplus
}
#endif
