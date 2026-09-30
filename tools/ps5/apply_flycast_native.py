#!/usr/bin/env python3
"""Deterministically adapt pinned Flycast for the native PS5 vmem/JIT layer."""

from __future__ import annotations
import argparse
import pathlib
import subprocess

EXPECTED = "e36e9df2dcc1487acdb1dc7725766f1f5ba029b5"

PS5_VMEM = r'''#include "types.h"
#include "hw/mem/addrspace.h"
#include "hw/sh4/sh4_if.h"
#include "oslib/virtmem.h"

#include <ps5rt/jit.hpp>
#include <ps5rt/c/shm.h>
#include <ps5rt/c/vmem.h>

#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace virtmem {
namespace {
ps5rt_shm g_ram{};
void* g_reserved_base = nullptr;
size_t g_reserved_size = 0;

std::mutex g_jit_lock;
std::unordered_map<void*, ps5rt::JitRegion> g_jit_regions;

constexpr size_t kAlign64K = 0x10000;

uintptr_t align_up(uintptr_t value, uintptr_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

bool remember_jit(ps5rt::JitRegion&& region) {
    std::scoped_lock lock(g_jit_lock);
    g_jit_regions.emplace(region.write_view.address, region);
    return true;
}

bool take_jit(void* rw, ps5rt::JitRegion& out) {
    std::scoped_lock lock(g_jit_lock);
    const auto it = g_jit_regions.find(rw);
    if (it == g_jit_regions.end())
        return false;
    out = it->second;
    g_jit_regions.erase(it);
    return true;
}
} // namespace

void destroy();\n\nbool region_lock(void* start, size_t len) {
    return ps5rt_vmem_protect(start, len, PS5RT_VMEM_READ) == 0;
}

bool region_unlock(void* start, size_t len) {
    return ps5rt_vmem_protect(
        start, len, PS5RT_VMEM_READ | PS5RT_VMEM_WRITE) == 0;
}

bool region_set_exec(void*, size_t) {
    return false;
}

bool init(void** vmem_base_addr, void** sh4rcb_addr, size_t ramSize) {
    if (!vmem_base_addr || !sh4rcb_addr || !ramSize)
        return false;

    if (ps5rt_shm_create(ramSize, &g_ram) != 0)
        return false;

    g_reserved_size = 512_MB + sizeof(Sh4RCB) + ARAM_SIZE_MAX + kAlign64K;
    if (ps5rt_vrange_reserve(g_reserved_size, nullptr, kAlign64K,
                             &g_reserved_base) != 0) {
        ps5rt_shm_destroy(&g_ram);
        return false;
    }

    const uintptr_t aligned =
        align_up(reinterpret_cast<uintptr_t>(g_reserved_base), kAlign64K);
    *sh4rcb_addr = reinterpret_cast<void*>(aligned);
    *vmem_base_addr =
        reinterpret_cast<void*>(aligned + sizeof(Sh4RCB));

    const size_t fpcb_size = sizeof(((Sh4RCB*)nullptr)->fpcb);
    void* sh4rcb_data = reinterpret_cast<void*>(aligned + fpcb_size);
    if (ps5rt_vmem_commit(
            sh4rcb_data, sizeof(Sh4RCB) - fpcb_size,
            PS5RT_VMEM_READ | PS5RT_VMEM_WRITE) != 0) {
        destroy();
        return false;
    }
    return true;
}

void destroy() {
    {
        std::scoped_lock lock(g_jit_lock);
        for (auto& [_, region] : g_jit_regions)
            (void)ps5rt::destroy_jit_region(region);
        g_jit_regions.clear();
    }

    if (g_reserved_base && g_reserved_size)
        (void)ps5rt_vrange_release(g_reserved_base, g_reserved_size);
    g_reserved_base = nullptr;
    g_reserved_size = 0;

    if (g_ram.handle)
        ps5rt_shm_destroy(&g_ram);
}

void reset_mem(void* ptr, unsigned size_bytes) {
    (void)ps5rt_vmem_decommit(ptr, size_bytes);
}

void ondemand_page(void* address, unsigned size_bytes) {
    verify(ps5rt_vmem_commit(
        address, size_bytes,
        PS5RT_VMEM_READ | PS5RT_VMEM_WRITE) == 0);
}

void create_mappings(const Mapping* maps, unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        if (!maps[i].memsize)
            continue;

        const u64 span = maps[i].end_address - maps[i].start_address;
        verify(span % maps[i].memsize == 0);
        const unsigned mirrors =
            static_cast<unsigned>(span / maps[i].memsize);

        for (unsigned j = 0; j < mirrors; ++j) {
            const u64 offset =
                maps[i].start_address + j * maps[i].memsize;
            void* target = &addrspace::ram_base[offset];
            unsigned flags = PS5RT_SHM_READ | PS5RT_SHM_FIXED;
            if (maps[i].allow_writes)
                flags |= PS5RT_SHM_WRITE;

            void* mapped = nullptr;
            const int rc = ps5rt_shm_map(
                &g_ram,
                static_cast<size_t>(maps[i].memoffset),
                static_cast<size_t>(maps[i].memsize),
                target, flags, PS5RT_SHM_FIXED, &mapped);
            verify(rc == 0 && mapped == target);
        }
    }
}

bool prepare_jit_block(void*, size_t size, void** code_area_rwx) {
    if (!code_area_rwx)
        return false;

    ps5rt::JitRegion region{};
    ps5rt::JitRequest request{
        size, 0x4000, "flycast-rwx-jit", false
    };
    if (!ps5rt::create_jit_region(request, region))
        return false;

    *code_area_rwx = region.write_view.address;
    return remember_jit(std::move(region));
}

bool prepare_jit_block(
        void*, size_t size, void** code_area_rw, ptrdiff_t* rx_offset) {
    if (!code_area_rw || !rx_offset)
        return false;

    ps5rt::JitRegion region{};
    ps5rt::JitRequest request{
        size, 0x4000, "flycast-dual-jit", true
    };
    if (!ps5rt::create_jit_region(request, region))
        return false;

    *code_area_rw = region.write_view.address;
    *rx_offset =
        static_cast<char*>(region.execute_view.address) -
        static_cast<char*>(region.write_view.address);
    return remember_jit(std::move(region));
}

void release_jit_block(void* code_area, size_t) {
    ps5rt::JitRegion region{};
    if (take_jit(code_area, region))
        (void)ps5rt::destroy_jit_region(region);
}

void release_jit_block(void* code_area_rw, void*, size_t) {
    release_jit_block(code_area_rw, 0);
}

void jit_set_exec(void*, size_t, bool) {}

void flush_cache(void*, void*, void*, void*) {}

} // namespace virtmem
'''

def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected 1 match, found {count}")
    return text.replace(old, new, 1)

def transform(root: pathlib.Path) -> dict[pathlib.Path, str]:
    out: dict[pathlib.Path, str] = {}

    vh = root / "core/oslib/virtmem.h"
    text = vh.read_text()
    text = replace_once(
        text,
        '#if defined(_WIN32) || defined(__APPLE__)\n#define DECLARE_CODE_CACHE(Name, Size) static u8 *Name;\n',
        '#if defined(__PROSPERO__)\n#define DECLARE_CODE_CACHE(Name, Size) static u8 *Name;\n'
        '#elif defined(_WIN32) || defined(__APPLE__)\n#define DECLARE_CODE_CACHE(Name, Size) static u8 *Name;\n',
        "virtmem prospero code cache",
    )
    out[vh] = text

    cm = root / "CMakeLists.txt"
    text = cm.read_text()
    text = replace_once(
        text,
        'option(USE_LIBCDIO "Use libcdio for CDROM access" OFF)\n',
        'option(USE_LIBCDIO "Use libcdio for CDROM access" OFF)\n'
        'option(FLYCAST_PS5 "Build native PlayStation 5 engine" OFF)\n'
        'if(FLYCAST_PS5)\n'
        '  add_compile_definitions(__PROSPERO__ FEAT_NO_RWX_PAGES)\n'
        '  add_compile_options(-march=znver2 -msse4.1 -mavx2 -mno-vzeroupper)\n'
        '  include_directories($ENV{NATIVE_EMUS_ROOT}/runtime/include)\n'
        'endif()\n',
        "CMake PS5 option",
    )
    text = replace_once(
        text,
        '''else()
\ttarget_sources(${PROJECT_NAME} PRIVATE
\t\t\tcore/linux/common.cpp
\t\t\tcore/linux/context.cpp
\t\t\tcore/linux/posix_vmem.cpp
\t\t\tcore/linux/unwind_info.cpp)
''',
        '''else()
\tif(FLYCAST_PS5)
\t\ttarget_sources(${PROJECT_NAME} PRIVATE
\t\t\tcore/linux/common.cpp
\t\t\tcore/linux/context.cpp
\t\t\tcore/linux/unwind_info.cpp
\t\t\tcore/ps5/ps5_vmem.cpp)
\telse()
\t\ttarget_sources(${PROJECT_NAME} PRIVATE
\t\t\tcore/linux/common.cpp
\t\t\tcore/linux/context.cpp
\t\t\tcore/linux/posix_vmem.cpp
\t\t\tcore/linux/unwind_info.cpp)
\tendif()
''',
        "CMake PS5 vmem source",
    )
    out[cm] = text
    out[root / "core/ps5/ps5_vmem.cpp"] = PS5_VMEM
    return out

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=pathlib.Path)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    root = args.source.resolve()

    head = subprocess.check_output(
        ["git", "-C", str(root), "rev-parse", "HEAD"], text=True
    ).strip()
    if head != EXPECTED:
        raise SystemExit(f"wrong Flycast revision: {head}; expected {EXPECTED}")
    if subprocess.run(["git", "-C", str(root), "diff", "--quiet"]).returncode:
        raise SystemExit("Flycast checkout has local modifications")

    try:
        transformed = transform(root)
    except RuntimeError as exc:
        raise SystemExit(str(exc))

    if args.check:
        print(f"Flycast {EXPECTED}: {len(transformed)} PS5 transformations matched")
        return 0

    for path, content in transformed.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
    (root / ".native-emus-ps5-flycast").write_text(EXPECTED + "\n")
    print(f"Flycast {EXPECTED}: PS5 dynarec/vmem transformation applied")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
