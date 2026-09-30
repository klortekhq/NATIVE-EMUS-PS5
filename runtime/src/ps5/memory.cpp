#include <ps5rt/jit.hpp>
#include <ps5rt/memory.hpp>

#include <ps5rt/c/exec.h>
#include <ps5rt/c/shm.h>
#include <ps5rt/c/vmem.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>

extern "C" {
int sceKernelMapFlexibleMemory(void** address, std::size_t size, int protection, int flags);
int sceKernelMapNamedFlexibleMemory(void** address, std::size_t size, int protection, int flags, const char* name);
int sceKernelReleaseFlexibleMemory(void* address, std::size_t size);
int sceKernelAvailableFlexibleMemorySize(unsigned long long* size);

long long sceKernelGetDirectMemorySize(void);
int sceKernelAllocateDirectMemory(long long search_start, long long search_end,
                                  unsigned long long size, unsigned long long alignment,
                                  int memory_type, long long* direct_start);
int sceKernelMapDirectMemory(void** address, unsigned long long size,
                             int protection, int flags, long long direct_start,
                             unsigned long long alignment);
int sceKernelReleaseDirectMemory(long long direct_start, unsigned long long size);
int sceKernelMprotect(const void* address, unsigned long long size, int protection);
int sceKernelMunmap(void* address, unsigned long long size);
int sceKernelReserveVirtualRange(void** address, unsigned long long size,
                                 int flags, unsigned long long alignment);

int sceKernelJitCreateSharedMemory(int flags, unsigned long long size,
                                   int protection, int* destination_handle);
int sceKernelJitMapSharedMemory(int handle, int protection, void** destination);
int sceKernelJitCreateAliasOfSharedMemory(int handle, int protection, int* destination_handle);
int sceKernelClose(int fd);
}

namespace {

constexpr std::size_t kPageSize = 0x4000;
constexpr std::size_t kLargeAlignment = 0x200000;
constexpr int kDirectMemoryTypeCached = 11;
constexpr int kDirectMemoryTypeCachedShared = 12;
constexpr int kFlexibleMapFixed = 0x1;
constexpr int kVirtualMapFixed = 0x10;
constexpr int kVirtualMapNoOverwrite = 0x80;
constexpr int kDirectMapFixed = 0x10;

constexpr int kProtRead = 1;
constexpr int kProtWrite = 2;
constexpr int kProtExec = 4;
constexpr int kProtRW = kProtRead | kProtWrite;
constexpr int kProtRWX = kProtRW | kProtExec;

std::size_t round_up(std::size_t value, std::size_t alignment) noexcept {
  if (alignment == 0) alignment = kPageSize;
  const auto rem = value % alignment;
  return rem == 0 ? value : value + (alignment - rem);
}

std::size_t normalize_alignment(std::size_t alignment) noexcept {
  if (alignment < kPageSize) return kPageSize;
  // PS5 mappings are page based. Callers asking for non-page multiples get
  // the next page multiple rather than an under-aligned mapping.
  return round_up(alignment, kPageSize);
}

void align_range(void* address,
                 std::size_t requested_size,
                 void*& page_address,
                 std::size_t& page_size) noexcept {
  const auto start = reinterpret_cast<std::uintptr_t>(address);
  const auto page_start = start & ~(static_cast<std::uintptr_t>(kPageSize) - 1);
  const auto end = start + requested_size;
  const auto page_end = round_up(end, kPageSize);
  page_address = reinterpret_cast<void*>(page_start);
  page_size = page_end - page_start;
}

int to_native_protection(ps5rt::Protection protection) noexcept {
  const auto bits = static_cast<std::uint8_t>(protection);
  int result = 0;
  if (bits & static_cast<std::uint8_t>(ps5rt::Protection::read)) result |= kProtRead;
  if (bits & static_cast<std::uint8_t>(ps5rt::Protection::write)) result |= kProtWrite;
  if (bits & static_cast<std::uint8_t>(ps5rt::Protection::execute)) result |= kProtExec;
  return result;
}

struct DirectRecord {
  long long start{-1};
  std::size_t size{};
};

struct ExecRecord {
  enum class Backend : std::uint8_t { jit_shared, direct } backend{Backend::jit_shared};
  int jit_handle{-1};
  long long direct_start{-1};
  std::size_t size{};
};

struct DualJitRecord {
  int primary_handle{-1};
  int alias_handle{-1};
  void* write_view{};
  void* execute_view{};
  std::size_t size{};
};

std::mutex g_registry_mutex;
std::unordered_map<void*, DirectRecord> g_direct_records;
std::unordered_map<void*, ExecRecord> g_exec_records;
std::unordered_map<void*, DualJitRecord> g_dual_jit_records;

int allocate_direct_block(std::size_t size,
                          std::size_t alignment,
                          int memory_type,
                          long long& out_start) noexcept {
  const long long total = sceKernelGetDirectMemorySize();
  if (total <= 0) return -1;

  const auto normalized_alignment = std::max(normalize_alignment(alignment), kPageSize);
  return sceKernelAllocateDirectMemory(
      0, total, static_cast<unsigned long long>(size),
      static_cast<unsigned long long>(normalized_alignment),
      memory_type, &out_start);
}

int map_direct_block(long long direct_start,
                     std::size_t size,
                     std::size_t alignment,
                     void* preferred,
                     bool fixed,
                     int protection,
                     void** out) noexcept {
  if (!out) return -1;

  void* address = preferred;
  const int initial_protection = protection & kProtExec
      ? (protection & ~kProtExec)
      : protection;

  const int rc = sceKernelMapDirectMemory(
      &address, static_cast<unsigned long long>(size),
      initial_protection, fixed ? kDirectMapFixed : 0,
      direct_start, static_cast<unsigned long long>(normalize_alignment(alignment)));
  if (rc != 0) return rc;

  if (fixed && address != preferred) {
    (void)sceKernelMunmap(address, static_cast<unsigned long long>(size));
    return -1;
  }

  if (protection & kProtExec) {
    const int protect_rc = sceKernelMprotect(
        address, static_cast<unsigned long long>(size), protection);
    if (protect_rc != 0) {
      (void)sceKernelMunmap(address, static_cast<unsigned long long>(size));
      return protect_rc;
    }
  }

  *out = address;
  return 0;
}

} // namespace

extern "C" void* ps5rt_exec_allocate(std::size_t requested_size, std::uintptr_t near_hint) {
  if (requested_size == 0) return nullptr;

  const std::size_t size = round_up(requested_size, kPageSize);

  // Preferred path: the kernel's executable-capable JIT shared-memory API.
  int handle = -1;
  int rc = sceKernelJitCreateSharedMemory(
      0, static_cast<unsigned long long>(size), kProtRWX, &handle);
  if (rc == 0 && handle >= 0) {
    void* mapped = near_hint ? reinterpret_cast<void*>(near_hint) : nullptr;
    // JitMapSharedMemory chooses its own address. Keep near_hint advisory only.
    mapped = nullptr;
    rc = sceKernelJitMapSharedMemory(handle, kProtRWX, &mapped);
    if (rc == 0 && mapped) {
      std::scoped_lock lock(g_registry_mutex);
      // Keep the JIT shared-memory handle alive for the lifetime of the
      // mapping. PS5SX2 does the same; closing it immediately is not assumed
      // to be safe across firmwares.
      g_exec_records[mapped] = {ExecRecord::Backend::jit_shared, handle, -1, size};
      return mapped;
    }
    (void)sceKernelClose(handle);
  } else if (handle >= 0) {
    (void)sceKernelClose(handle);
  }

  // Fallback: direct memory, initially RW then promoted to RWX. This keeps
  // large JIT caches outside the flexible-memory budget.
  long long direct_start = -1;
  const std::size_t alignment =
      size >= kLargeAlignment ? kLargeAlignment : kPageSize;
  if (allocate_direct_block(size, alignment, kDirectMemoryTypeCachedShared, direct_start) != 0)
    return nullptr;

  void* mapped = nullptr;
  if (map_direct_block(direct_start, size, alignment,
                       near_hint ? reinterpret_cast<void*>(near_hint) : nullptr,
                       false, kProtRWX, &mapped) != 0) {
    (void)sceKernelReleaseDirectMemory(
        direct_start, static_cast<unsigned long long>(size));
    return nullptr;
  }

  {
    std::scoped_lock lock(g_registry_mutex);
    g_exec_records[mapped] = {ExecRecord::Backend::direct, -1, direct_start, size};
  }
  return mapped;
}

extern "C" void ps5rt_exec_release(void* ptr) {
  if (!ptr) return;

  ExecRecord record{};
  {
    std::scoped_lock lock(g_registry_mutex);
    const auto it = g_exec_records.find(ptr);
    if (it == g_exec_records.end()) return;
    record = it->second;
    g_exec_records.erase(it);
  }

  (void)sceKernelMunmap(ptr, static_cast<unsigned long long>(record.size));
  if (record.backend == ExecRecord::Backend::jit_shared) {
    if (record.jit_handle >= 0)
      (void)sceKernelClose(record.jit_handle);
  } else if (record.direct_start >= 0) {
    (void)sceKernelReleaseDirectMemory(
        record.direct_start, static_cast<unsigned long long>(record.size));
  }
}

extern "C" int ps5rt_shm_create(std::size_t requested_size, ps5rt_shm* out) {
  if (!out || requested_size == 0) return -1;
  *out = {};

  const std::size_t size = round_up(requested_size, kPageSize);
  long long direct_start = -1;
  const int rc = allocate_direct_block(size, kPageSize, kDirectMemoryTypeCached, direct_start);
  if (rc != 0) return rc;

  out->handle = static_cast<std::uint64_t>(direct_start);
  out->size = size;
  return 0;
}

extern "C" void ps5rt_shm_destroy(ps5rt_shm* shm) {
  if (!shm || shm->size == 0) return;
  const auto direct_start = static_cast<long long>(shm->handle);
  (void)sceKernelReleaseDirectMemory(
      direct_start, static_cast<unsigned long long>(shm->size));
  *shm = {};
}

extern "C" int ps5rt_shm_map(ps5rt_shm* shm,
                              std::size_t offset,
                              std::size_t requested_size,
                              void* hint,
                              unsigned flags,
                              unsigned map_flags,
                              void** out) {
  if (!shm || !out || requested_size == 0) return -1;
  if (offset % kPageSize != 0) return -1;

  const std::size_t size = round_up(requested_size, kPageSize);
  if (offset > shm->size || size > shm->size - offset) return -1;

  int protection = 0;
  if (flags & PS5RT_SHM_READ) protection |= kProtRead;
  if (flags & PS5RT_SHM_WRITE) protection |= kProtWrite;
  if (flags & PS5RT_SHM_EXEC) protection |= kProtExec;
  if (protection == 0) return -1;

  const auto base = static_cast<long long>(shm->handle);
  const bool fixed = (flags & PS5RT_SHM_FIXED) != 0 ||
                     (map_flags & PS5RT_SHM_FIXED) != 0;
  if (fixed && !hint) return -1;

  return map_direct_block(
      base + static_cast<long long>(offset), size, kPageSize,
      hint, fixed, protection, out);
}

extern "C" int ps5rt_shm_unmap(void* address, std::size_t requested_size, unsigned flags) {
  if (!address || requested_size == 0) return -1;
  const std::size_t size = round_up(requested_size, kPageSize);

  const int rc = sceKernelMunmap(address, static_cast<unsigned long long>(size));
  if (rc != 0) return rc;

  if (flags & PS5RT_SHM_KEEP_RESERVED) {
    void* reserved = address;
    const int reserve_rc = sceKernelReserveVirtualRange(
        &reserved, static_cast<unsigned long long>(size),
        kVirtualMapFixed, kPageSize);
    if (reserve_rc != 0) return reserve_rc;
    if (reserved != address) {
      (void)sceKernelMunmap(reserved, static_cast<unsigned long long>(size));
      return -1;
    }
  }
  return 0;
}

extern "C" int ps5rt_vrange_reserve(std::size_t requested_size,
                                     void* hint,
                                     std::size_t alignment,
                                     void** out) {
  if (!out || requested_size == 0) return -1;

  const auto normalized_alignment = normalize_alignment(alignment);
  const auto size = round_up(requested_size, kPageSize);
  void* address = hint;

  // A non-null hint is preferred rather than required. If the exact range is
  // unavailable, retry without MAP_FIXED so large fastmem reservations can
  // still succeed at another legal address.
  int rc = sceKernelReserveVirtualRange(
      &address, static_cast<unsigned long long>(size),
      hint ? (kVirtualMapFixed | kVirtualMapNoOverwrite) : 0,
      static_cast<unsigned long long>(normalized_alignment));
  if (rc != 0 && hint) {
    address = nullptr;
    rc = sceKernelReserveVirtualRange(
        &address, static_cast<unsigned long long>(size), 0,
        static_cast<unsigned long long>(normalized_alignment));
  }
  if (rc != 0) return rc;

  *out = address;
  return 0;
}

extern "C" int ps5rt_vrange_release(void* address, std::size_t requested_size) {
  if (!address || requested_size == 0) return -1;
  const auto size = round_up(requested_size, kPageSize);
  return sceKernelMunmap(address, static_cast<unsigned long long>(size));
}


extern "C" int ps5rt_vmem_commit(void* address,
                                  std::size_t requested_size,
                                  unsigned flags) {
  if (!address || requested_size == 0) return -1;

  void* page_address = nullptr;
  std::size_t size = 0;
  align_range(address, requested_size, page_address, size);

  int protection = 0;
  if (flags & PS5RT_VMEM_READ) protection |= kProtRead;
  if (flags & PS5RT_VMEM_WRITE) protection |= kProtWrite;
  if (flags & PS5RT_VMEM_EXEC) protection |= kProtExec;
  if (protection == 0) return -1;

  void* mapped = page_address;
  int rc = sceKernelMapFlexibleMemory(
      &mapped, size, protection, kFlexibleMapFixed);
  if (rc != 0) return rc;
  if (mapped != page_address) {
    (void)sceKernelReleaseFlexibleMemory(mapped, size);
    return -1;
  }
  return 0;
}

extern "C" int ps5rt_vmem_decommit(void* address,
                                    std::size_t requested_size) {
  if (!address || requested_size == 0) return -1;

  void* page_address = nullptr;
  std::size_t size = 0;
  align_range(address, requested_size, page_address, size);

  int rc = sceKernelReleaseFlexibleMemory(page_address, size);
  if (rc != 0) return rc;

  void* reserved = page_address;
  rc = sceKernelReserveVirtualRange(
      &reserved, size, kVirtualMapFixed, kPageSize);
  if (rc != 0) return rc;
  if (reserved != page_address) {
    (void)sceKernelMunmap(reserved, size);
    return -1;
  }
  return 0;
}

extern "C" int ps5rt_vmem_protect(void* address,
                                   std::size_t requested_size,
                                   unsigned flags) {
  if (!address || requested_size == 0) return -1;

  void* page_address = nullptr;
  std::size_t size = 0;
  align_range(address, requested_size, page_address, size);

  int protection = 0;
  if (flags & PS5RT_VMEM_READ) protection |= kProtRead;
  if (flags & PS5RT_VMEM_WRITE) protection |= kProtWrite;
  if (flags & PS5RT_VMEM_EXEC) protection |= kProtExec;

  return sceKernelMprotect(page_address, size, protection);
}

namespace ps5rt {

Result allocate_memory(MemoryKind kind,
                       const MemoryRequest& request,
                       Mapping& out) noexcept {
  out = {};
  if (request.size == 0)
    return {ErrorCode::invalid_argument, 0, "memory size is zero"};

  const auto size = round_up(request.size, kPageSize);
  const auto alignment = normalize_alignment(request.alignment);
  const int protection = to_native_protection(request.protection);
  if (protection == 0)
    return {ErrorCode::invalid_argument, 0, "memory protection is empty"};

  if (kind == MemoryKind::executable) {
    void* address = ps5rt_exec_allocate(
        size, reinterpret_cast<std::uintptr_t>(request.preferred_address));
    if (!address)
      return {ErrorCode::out_of_memory, 0, "executable memory allocation failed"};
    if (request.fixed_address && address != request.preferred_address) {
      ps5rt_exec_release(address);
      return {ErrorCode::system_error, 0, "fixed executable mapping unavailable"};
    }
    out = {address, size, kind, request.protection};
    return Result::success();
  }

  if (kind == MemoryKind::flexible) {
    void* address = request.preferred_address;
    const int flags = request.fixed_address ? kFlexibleMapFixed : 0;
    int rc = 0;
    if (request.debug_name && *request.debug_name) {
      rc = sceKernelMapNamedFlexibleMemory(
          &address, size, protection, flags, request.debug_name);
    } else {
      rc = sceKernelMapFlexibleMemory(&address, size, protection, flags);
    }
    if (rc != 0)
      return {ErrorCode::out_of_memory, rc, "flexible memory mapping failed"};
    if (request.fixed_address && address != request.preferred_address) {
      (void)sceKernelReleaseFlexibleMemory(address, size);
      return {ErrorCode::system_error, 0, "fixed flexible mapping moved"};
    }
    out = {address, size, kind, request.protection};
    return Result::success();
  }

  if (kind == MemoryKind::direct) {
    long long direct_start = -1;
    int rc = allocate_direct_block(size, alignment, kDirectMemoryTypeCached, direct_start);
    if (rc != 0)
      return {ErrorCode::out_of_memory, rc, "direct memory allocation failed"};

    void* address = nullptr;
    rc = map_direct_block(
        direct_start, size, alignment, request.preferred_address,
        request.fixed_address, protection, &address);
    if (rc != 0) {
      (void)sceKernelReleaseDirectMemory(direct_start, size);
      return {ErrorCode::system_error, rc, "direct memory mapping failed"};
    }
    {
      std::scoped_lock lock(g_registry_mutex);
      g_direct_records[address] = {direct_start, size};
    }
    out = {address, size, kind, request.protection};
    return Result::success();
  }

  return {ErrorCode::unsupported, 0, "pooled memory backend not enabled yet"};
}

Result release_memory(Mapping& mapping) noexcept {
  if (!mapping) return Result::success();

  if (mapping.kind == MemoryKind::executable) {
    ps5rt_exec_release(mapping.address);
    mapping = {};
    return Result::success();
  }

  if (mapping.kind == MemoryKind::flexible) {
    const int rc = sceKernelReleaseFlexibleMemory(mapping.address, mapping.size);
    if (rc != 0)
      return {ErrorCode::system_error, rc, "flexible memory release failed"};
    mapping = {};
    return Result::success();
  }

  if (mapping.kind == MemoryKind::direct) {
    DirectRecord record{};
    {
      std::scoped_lock lock(g_registry_mutex);
      const auto it = g_direct_records.find(mapping.address);
      if (it == g_direct_records.end())
        return {ErrorCode::invalid_argument, 0, "unknown direct mapping"};
      record = it->second;
      g_direct_records.erase(it);
    }

    int rc = sceKernelMunmap(mapping.address, record.size);
    if (rc == 0)
      rc = sceKernelReleaseDirectMemory(record.start, record.size);
    if (rc != 0)
      return {ErrorCode::system_error, rc, "direct memory release failed"};

    mapping = {};
    return Result::success();
  }

  return {ErrorCode::unsupported, 0, "pooled memory release not enabled"};
}

Result query_available_memory(MemoryKind kind, std::size_t& out_bytes) noexcept {
  out_bytes = 0;
  if (kind != MemoryKind::flexible)
    return {ErrorCode::unsupported, 0, "available-size query not implemented for this memory kind"};

  unsigned long long bytes = 0;
  const int rc = sceKernelAvailableFlexibleMemorySize(&bytes);
  if (rc != 0)
    return {ErrorCode::system_error, rc, "flexible memory size query failed"};
  out_bytes = static_cast<std::size_t>(bytes);
  return Result::success();
}

Result create_jit_region(const JitRequest& request, JitRegion& out) noexcept {
  out = {};
  if (request.size == 0)
    return {ErrorCode::invalid_argument, 0, "JIT size is zero"};

  const auto size = round_up(request.size, kPageSize);

  if (!request.prefer_dual_mapping) {
    void* address = ps5rt_exec_allocate(size, 0);
    if (!address)
      return {ErrorCode::out_of_memory, 0, "JIT allocation failed"};

    const auto protection = Protection::read | Protection::write | Protection::execute;
    out.write_view = {address, size, MemoryKind::executable, protection};
    out.execute_view = out.write_view;
    return Result::success();
  }

  int primary = -1;
  int alias = -1;
  void* write_view = nullptr;
  void* execute_view = nullptr;

  int rc = sceKernelJitCreateSharedMemory(
      0, static_cast<unsigned long long>(size), kProtRWX, &primary);
  if (rc != 0 || primary < 0)
    return {ErrorCode::out_of_memory, rc, "JIT shared-memory creation failed"};

  rc = sceKernelJitMapSharedMemory(primary, kProtRW, &write_view);
  if (rc == 0)
    rc = sceKernelJitCreateAliasOfSharedMemory(primary, kProtRead | kProtExec, &alias);
  if (rc == 0)
    rc = sceKernelJitMapSharedMemory(alias, kProtRead | kProtExec, &execute_view);

  if (rc != 0 || !write_view || !execute_view) {
    if (write_view)
      (void)sceKernelMunmap(write_view, static_cast<unsigned long long>(size));
    if (execute_view)
      (void)sceKernelMunmap(execute_view, static_cast<unsigned long long>(size));
    if (alias >= 0)
      (void)sceKernelClose(alias);
    (void)sceKernelClose(primary);
    return {ErrorCode::system_error, rc, "dual-view JIT mapping failed"};
  }

  const DualJitRecord record{primary, alias, write_view, execute_view, size};
  {
    std::scoped_lock lock(g_registry_mutex);
    g_dual_jit_records[write_view] = record;
    g_dual_jit_records[execute_view] = record;
  }

  out.write_view = {
      write_view, size, MemoryKind::executable,
      Protection::read | Protection::write};
  out.execute_view = {
      execute_view, size, MemoryKind::executable,
      Protection::read | Protection::execute};
  return Result::success();
}

Result destroy_jit_region(JitRegion& region) noexcept {
  if (!region) return Result::success();

  DualJitRecord dual{};
  bool is_dual = false;
  {
    std::scoped_lock lock(g_registry_mutex);
    auto it = g_dual_jit_records.find(region.execute_view.address);
    if (it == g_dual_jit_records.end())
      it = g_dual_jit_records.find(region.write_view.address);
    if (it != g_dual_jit_records.end()) {
      dual = it->second;
      g_dual_jit_records.erase(dual.write_view);
      g_dual_jit_records.erase(dual.execute_view);
      is_dual = true;
    }
  }

  if (is_dual) {
    int first_error = 0;
    if (dual.write_view) {
      const int rc = sceKernelMunmap(
          dual.write_view, static_cast<unsigned long long>(dual.size));
      if (rc != 0 && first_error == 0) first_error = rc;
    }
    if (dual.execute_view && dual.execute_view != dual.write_view) {
      const int rc = sceKernelMunmap(
          dual.execute_view, static_cast<unsigned long long>(dual.size));
      if (rc != 0 && first_error == 0) first_error = rc;
    }
    if (dual.alias_handle >= 0)
      (void)sceKernelClose(dual.alias_handle);
    if (dual.primary_handle >= 0)
      (void)sceKernelClose(dual.primary_handle);
    region = {};
    if (first_error != 0)
      return {ErrorCode::system_error, first_error, "dual-view JIT unmap failed"};
    return Result::success();
  }

  void* const address = region.execute_view.address
      ? region.execute_view.address
      : region.write_view.address;
  ps5rt_exec_release(address);
  region = {};
  return Result::success();
}

Result flush_instruction_cache(void* address, std::size_t size) noexcept {
  if (!address || size == 0)
    return {ErrorCode::invalid_argument, 0, "invalid instruction-cache range"};

#if defined(__GNUC__) || defined(__clang__)
  auto* begin = static_cast<char*>(address);
  __builtin___clear_cache(begin, begin + size);
#endif
  return Result::success();
}

} // namespace ps5rt
