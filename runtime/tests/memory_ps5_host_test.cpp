#include <ps5rt/jit.hpp>
#include <ps5rt/memory.hpp>
#include <ps5rt/c/exec.h>
#include <ps5rt/c/jit.h>
#include <ps5rt/c/shm.h>
#include <ps5rt/c/vmem.h>

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <unordered_map>

namespace {
int next_fd = 10;
long long next_direct = 0x200000;
std::unordered_map<int, std::size_t> jit_handles;
std::unordered_map<long long, std::size_t> direct_blocks;
std::unordered_map<void*, std::size_t> mappings;
std::unordered_map<void*, bool> mapping_owned;
}

extern "C" int sceKernelJitCreateSharedMemory(int, unsigned long long size, int, int* out) {
  *out = next_fd++;
  jit_handles[*out] = static_cast<std::size_t>(size);
  return 0;
}
extern "C" int sceKernelJitCreateAliasOfSharedMemory(int source, int, int* out) {
  auto it = jit_handles.find(source);
  if (it == jit_handles.end()) return -1;
  *out = next_fd++;
  jit_handles[*out] = it->second;
  return 0;
}
extern "C" int sceKernelJitMapSharedMemory(int fd, int, void** out) {
  auto it = jit_handles.find(fd);
  if (it == jit_handles.end()) return -1;
  *out = std::malloc(it->second);
  if (!*out) return -1;
  mapping_owned[*out] = true;
  mappings[*out] = it->second;
  return 0;
}
extern "C" int sceKernelClose(int fd) {
  jit_handles.erase(fd);
  return 0;
}

extern "C" int sceKernelMapFlexibleMemory(void** out, std::size_t size, int, int) {
  const bool owned = (*out == nullptr);
  if (!*out) *out = std::malloc(size);
  if (!*out) return -1;
  mapping_owned[*out] = owned;
  mappings[*out] = size;
  return 0;
}
extern "C" int sceKernelMapNamedFlexibleMemory(void** out, std::size_t size, int prot, int flags, const char*) {
  return sceKernelMapFlexibleMemory(out, size, prot, flags);
}
extern "C" int sceKernelReleaseFlexibleMemory(void* p, std::size_t) {
  auto it = mappings.find(p);
  if (it != mappings.end()) {
    if (mapping_owned[p]) std::free(p);
    mapping_owned.erase(p);
    mappings.erase(it);
  }
  return 0;
}
extern "C" int sceKernelAvailableFlexibleMemorySize(unsigned long long* out) {
  *out = 256ull * 1024 * 1024;
  return 0;
}

extern "C" long long sceKernelGetDirectMemorySize(void) {
  return 2ll * 1024 * 1024 * 1024;
}
extern "C" int sceKernelAllocateDirectMemory(long long, long long, unsigned long long size,
                                              unsigned long long, int, long long* out) {
  *out = next_direct;
  next_direct += static_cast<long long>(size);
  direct_blocks[*out] = static_cast<std::size_t>(size);
  return 0;
}
extern "C" int sceKernelMapDirectMemory(void** out, unsigned long long size, int, int,
                                         long long, unsigned long long) {
  const bool owned = (*out == nullptr);
  if (!*out) *out = std::malloc(static_cast<std::size_t>(size));
  if (!*out) return -1;
  mapping_owned[*out] = owned;
  mappings[*out] = static_cast<std::size_t>(size);
  return 0;
}
extern "C" int sceKernelReleaseDirectMemory(long long start, unsigned long long) {
  direct_blocks.erase(start);
  return 0;
}
extern "C" int sceKernelMprotect(const void*, unsigned long long, int) {
  return 0;
}
extern "C" int sceKernelMunmap(void* p, unsigned long long) {
  auto it = mappings.find(p);
  if (it != mappings.end()) {
    if (mapping_owned[p]) std::free(p);
    mapping_owned.erase(p);
    mappings.erase(it);
  }
  return 0;
}
extern "C" int sceKernelReserveVirtualRange(void** out, unsigned long long size, int, unsigned long long) {
  const bool owned = (*out == nullptr);
  if (!*out) *out = std::malloc(static_cast<std::size_t>(size));
  if (!*out) return -1;
  mapping_owned[*out] = owned;
  mappings[*out] = static_cast<std::size_t>(size);
  return 0;
}

int main() {
  {
    void* fixed = nullptr;
    void* requested = reinterpret_cast<void*>(
        static_cast<std::uintptr_t>(0x80000000ull));
    assert(ps5rt_vrange_reserve_fixed(
               0x20000000ull, requested, 0x4000, &fixed) == 0);
    assert(fixed == requested);
    assert(mappings.contains(requested));
    assert(ps5rt_vrange_release(fixed, 0x20000000ull) == 0);
    assert(!mappings.contains(requested));
  }

  {
    ps5rt::JitRegion region{};
    ps5rt::JitRequest request{};
    request.size = 16 * 1024 * 1024;
    request.alignment = 16 * 1024;
    request.debug_name = "fixed-dual-jit";
    request.prefer_dual_mapping = true;
    request.preferred_execute_address =
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(0xA0000000ull));
    request.preferred_write_address =
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(0xB0000000ull));
    request.require_fixed_execute = true;
    request.require_fixed_write = true;

    assert(ps5rt::create_jit_region(request, region));
    assert(region.execute_view.address == request.preferred_execute_address);
    assert(region.write_view.address == request.preferred_write_address);
    assert(ps5rt::destroy_jit_region(region));
  }

  {
    ps5rt::JitRegion region{};
    ps5rt::JitRequest request{64 * 1024, 16 * 1024, "dual-jit", true};
    assert(ps5rt::create_jit_region(request, region));
    assert(region.write_view.address);
    assert(region.execute_view.address);
    assert(region.write_view.address != region.execute_view.address);
    assert(jit_handles.size() == 2);
    assert(ps5rt::destroy_jit_region(region));
    assert(jit_handles.empty());
    assert(mappings.empty());
  }

  {
    ps5rt_jit_region c_region{};
    assert(ps5rt_jit_create(
               128 * 1024, 16 * 1024, PS5RT_JIT_DUAL_VIEW, &c_region) == 0);
    assert(c_region.write_view);
    assert(c_region.execute_view);
    assert(c_region.write_view != c_region.execute_view);
    assert(c_region.size >= 128 * 1024);
    assert(ps5rt_jit_flush(&c_region, 0, 4096) == 0);
    assert(ps5rt_jit_destroy(&c_region) == 0);
    assert(!c_region.write_view && !c_region.execute_view && c_region.size == 0);
    assert(jit_handles.empty());
    assert(mappings.empty());
  }

  {
    void* code = ps5rt_exec_allocate(64 * 1024, 0);
    assert(code);
    assert(jit_handles.size() == 1);
    ps5rt_exec_release(code);
    assert(jit_handles.empty());
    assert(mappings.empty());
  }

  {
    ps5rt_shm shm{};
    assert(ps5rt_shm_create(3 * 1024 * 1024, &shm) == 0);
    void* view = nullptr;
    assert(ps5rt_shm_map(&shm, 0, 0x4000, nullptr,
                         PS5RT_SHM_READ | PS5RT_SHM_WRITE, 0, &view) == 0);
    assert(view);
    assert(ps5rt_shm_unmap(view, 0x4000, 0) == 0);
    ps5rt_shm_destroy(&shm);
    assert(shm.handle == 0);
    assert(direct_blocks.empty());
  }

  {
    std::size_t bytes = 0;
    assert(ps5rt::query_available_memory(ps5rt::MemoryKind::flexible, bytes));
    assert(bytes == 256ull * 1024 * 1024);
  }

  return 0;
}
