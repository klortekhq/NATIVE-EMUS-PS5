#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// PS5 executable-memory contract used by emulator JITs.
// The concrete backend is provided by ps5rt_ps5; emulator source should not
// call Sony APIs directly when this abstraction is sufficient.
void *ps5rt_exec_allocate(size_t size, uintptr_t near_hint);
void ps5rt_exec_release(void *ptr);

#ifdef __cplusplus
}
#endif
