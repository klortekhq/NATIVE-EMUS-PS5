#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  PS5RT_VMEM_READ  = 1u << 0,
  PS5RT_VMEM_WRITE = 1u << 1,
  PS5RT_VMEM_EXEC  = 1u << 2,
};

// Reserve an address range without committing physical pages. The hint form
// may fall back to another address if the requested range is unavailable.
int ps5rt_vrange_reserve(size_t size, void* hint, size_t alignment, void** out);

// Strict fixed-address reservation. Success guarantees *out == address.
int ps5rt_vrange_reserve_fixed(
    size_t size, void* address, size_t alignment, void** out);

// Release a previously reserved virtual range.
int ps5rt_vrange_release(void* address, size_t size);

// Commit flexible-memory pages at an address that is already part of a
// reserved virtual range. The mapping is fixed: success guarantees the same
// address.
int ps5rt_vmem_commit(void* address, size_t size, unsigned protection);

// Release committed flexible pages and restore a PROT_NONE-style virtual
// reservation at the same address.
int ps5rt_vmem_decommit(void* address, size_t size);

// Change CPU protection on an already committed/mapped range.
int ps5rt_vmem_protect(void* address, size_t size, unsigned protection);

#ifdef __cplusplus
}
#endif
