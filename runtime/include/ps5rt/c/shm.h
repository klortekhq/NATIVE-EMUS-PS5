#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ps5rt_shm {
  uint64_t handle;
  size_t size;
} ps5rt_shm;

enum {
  PS5RT_SHM_READ  = 1u << 0,
  PS5RT_SHM_WRITE = 1u << 1,
  PS5RT_SHM_EXEC  = 1u << 2,
  PS5RT_SHM_FIXED = 1u << 3,
  PS5RT_SHM_KEEP_RESERVED = 1u << 4,
};

int ps5rt_shm_create(size_t size, ps5rt_shm *out);
void ps5rt_shm_destroy(ps5rt_shm *shm);
int ps5rt_shm_map(ps5rt_shm *shm, size_t offset, size_t size, void *hint,
                  unsigned flags, unsigned map_flags, void **out);
int ps5rt_shm_unmap(void *address, size_t size, unsigned flags);

int ps5rt_vrange_reserve(size_t size, void *hint, size_t alignment, void **out);
int ps5rt_vrange_release(void *address, size_t size);

#ifdef __cplusplus
}
#endif
