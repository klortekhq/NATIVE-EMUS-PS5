#!/usr/bin/env python3
"""Deterministically adapt PPSSPP v1.20.4 for the native PS5 engine build.

This is intentionally an exact-source transformer rather than a fuzzy patch:
every replacement must match exactly once. If upstream/source drift is detected,
the script aborts before writing any file.
"""

from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys

EXPECTED = "fa50bb1976065c4f8b1b47af227d367fe9771555"


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one exact match, found {count}")
    return text.replace(old, new, 1)


def transform(root: pathlib.Path) -> dict[pathlib.Path, str]:
    out: dict[pathlib.Path, str] = {}

    path = root / "ppsspp_config.h"
    text = path.read_text()
    text = replace_once(
        text,
        """#elif defined(__linux__)
    #define PPSSPP_PLATFORM_LINUX 1
#elif defined(__OpenBSD__)
    #define PPSSPP_PLATFORM_OPENBSD 1
#endif""",
        """#elif defined(__linux__)
    #define PPSSPP_PLATFORM_LINUX 1
#elif defined(__PROSPERO__)
    #define PPSSPP_PLATFORM_PS5 1
#elif defined(__OpenBSD__)
    #define PPSSPP_PLATFORM_OPENBSD 1
#endif""",
        "ppsspp_config platform",
    )
    text = replace_once(
        text,
        """#if !PPSSPP_PLATFORM(WINDOWS) || ((!PPSSPP_ARCH(ARM) && !PPSSPP_ARCH(ARM64)) && !PPSSPP_PLATFORM(UWP))
#define PPSSPP_API_ANY_GL 1
#endif""",
        """#if (!PPSSPP_PLATFORM(WINDOWS) || ((!PPSSPP_ARCH(ARM) && !PPSSPP_ARCH(ARM64)) && !PPSSPP_PLATFORM(UWP))) && !PPSSPP_PLATFORM(PS5)
#define PPSSPP_API_ANY_GL 1
#endif""",
        "ppsspp_config GL",
    )
    out[path] = text

    path = root / "CMakeLists.txt"
    text = path.read_text()
    text = replace_once(text, "if(NOT OPENGL_LIBRARIES)\n", "if(NOT OPENGL_LIBRARIES AND NOT PPSSPP_PS5)\n", "CMake OpenGL")
    text = replace_once(
        text,
        "target_compile_definitions(Common PRIVATE Z7_CRC_NUM_TABLES=1)\n",
        """target_compile_definitions(Common PRIVATE Z7_CRC_NUM_TABLES=1)

if(PPSSPP_PS5)
    file(GLOB PS5_GL_SOURCES ${CMAKE_SOURCE_DIR}/Common/GPU/OpenGL/*.cpp ${CMAKE_SOURCE_DIR}/Common/GPU/OpenGL/*.c)
    set_source_files_properties(${PS5_GL_SOURCES} TARGET_DIRECTORY Common PROPERTIES HEADER_FILE_ONLY TRUE)
endif()
""",
        "CMake Common GL exclusion",
    )
    text = replace_once(
        text,
        'elseif(${CMAKE_SYSTEM_NAME} MATCHES "^(DragonFly|FreeBSD|NetBSD)$")\n\ttarget_link_libraries(native execinfo)\n',
        'elseif(${CMAKE_SYSTEM_NAME} MATCHES "^(DragonFly|FreeBSD|NetBSD)$" AND NOT PPSSPP_PS5)\n\ttarget_link_libraries(native execinfo)\n',
        "CMake execinfo",
    )
    text = replace_once(
        text,
        "set(GPU_IMPLS ${GPU_GLES} ${GPU_VULKAN})\n",
        """set(GPU_IMPLS ${GPU_VULKAN})
if(NOT PPSSPP_PS5)
    list(APPEND GPU_IMPLS ${GPU_GLES})
endif()
""",
        "CMake Vulkan-only",
    )
    out[path] = text

    path = root / "Common/CPUDetect.cpp"
    text = path.read_text()
    text = replace_once(
        text,
        """#ifdef __ANDROID__
#include <sys/stat.h>
#include <fcntl.h>
#elif PPSSPP_PLATFORM(MAC)
#include <sys/sysctl.h>
#endif""",
        """#ifdef __ANDROID__
#include <sys/stat.h>
#include <fcntl.h>
#elif PPSSPP_PLATFORM(MAC)
#include <sys/sysctl.h>
#elif PPSSPP_PLATFORM(PS5)
#include <unistd.h>
#endif""",
        "CPUDetect include",
    )
    text = replace_once(
        text,
        """#elif PPSSPP_PLATFORM(MAC)
	int num = 0;
	size_t sz = sizeof(num);
	if (sysctlbyname("hw.physicalcpu_max", &num, &sz, nullptr, 0) == 0) {
		num_cores = num;
		sz = sizeof(num);
		if (sysctlbyname("hw.logicalcpu_max", &num, &sz, nullptr, 0) == 0) {
			logical_cpu_count = num / std::max(num_cores, 1);
		}
	}
#endif""",
        """#elif PPSSPP_PLATFORM(MAC)
	int num = 0;
	size_t sz = sizeof(num);
	if (sysctlbyname("hw.physicalcpu_max", &num, &sz, nullptr, 0) == 0) {
		num_cores = num;
		sz = sizeof(num);
		if (sysctlbyname("hw.logicalcpu_max", &num, &sz, nullptr, 0) == 0) {
			logical_cpu_count = num / std::max(num_cores, 1);
		}
	}
#elif PPSSPP_PLATFORM(PS5)
	long online = sysconf(_SC_NPROCESSORS_ONLN);
	if (online > 0) {
		num_cores = static_cast<int>(online);
		logical_cpu_count = 1;
	}
#endif""",
        "CPUDetect PS5 count",
    )
    out[path] = text

    path = root / "Common/MemArena.h"
    text = path.read_text()
    text = replace_once(
        text,
        '#include "Common/CommonTypes.h"\n',
        '#include "ppsspp_config.h"\n#include "Common/CommonTypes.h"\n#if PPSSPP_PLATFORM(PS5)\n#include <ps5rt/c/shm.h>\n#endif\n',
        "MemArena includes",
    )
    text = replace_once(
        text,
        """#else
	int fd = -1;
#endif
};""",
        """#else
	int fd = -1;
#endif
#if PPSSPP_PLATFORM(PS5)
	ps5rt_shm shm_{};
#endif
};""",
        "MemArena PS5 state",
    )
    out[path] = text

    path = root / "Common/MemArenaPosix.cpp"
    text = path.read_text()
    text = replace_once(
        text,
        """bool MemArena::GrabMemSpace(size_t size) {
#ifndef NO_MMAP""",
        """bool MemArena::GrabMemSpace(size_t size) {
#if PPSSPP_PLATFORM(PS5)
	return ps5rt_shm_create(size, &shm_) == 0;
#endif
#ifndef NO_MMAP""",
        "MemArena GrabMemSpace",
    )
    text = replace_once(
        text,
        """void MemArena::ReleaseSpace() {
#ifndef NO_MMAP
	close(fd);
#endif
}""",
        """void MemArena::ReleaseSpace() {
#if PPSSPP_PLATFORM(PS5)
	ps5rt_shm_destroy(&shm_);
	return;
#endif
#ifndef NO_MMAP
	close(fd);
#endif
}""",
        "MemArena ReleaseSpace",
    )
    text = replace_once(
        text,
        """void *MemArena::CreateView(s64 offset, size_t size, void *base)
{
#ifdef NO_MMAP
    return (void*) base;
#else""",
        """void *MemArena::CreateView(s64 offset, size_t size, void *base)
{
#if PPSSPP_PLATFORM(PS5)
	void *view = nullptr;
	unsigned flags = PS5RT_SHM_READ | PS5RT_SHM_WRITE;
	if (base)
		flags |= PS5RT_SHM_FIXED;
	return ps5rt_shm_map(&shm_, static_cast<size_t>(offset), size, base, flags, 0, &view) == 0 ? view : nullptr;
#elif defined(NO_MMAP)
    return (void*) base;
#else""",
        "MemArena CreateView",
    )
    text = replace_once(
        text,
        """void MemArena::ReleaseView(s64 offset, void* view, size_t size) {
#ifndef NO_MMAP
	munmap(view, size);
#endif
}""",
        """void MemArena::ReleaseView(s64 offset, void* view, size_t size) {
#if PPSSPP_PLATFORM(PS5)
	(void)offset;
	(void)ps5rt_shm_unmap(view, size, PS5RT_SHM_KEEP_RESERVED);
#elif !defined(NO_MMAP)
	munmap(view, size);
#endif
}""",
        "MemArena ReleaseView",
    )
    text = replace_once(
        text,
        """u8* MemArena::Find4GBBase() {
	// Now, create views in high memory where there's plenty of space.
#if PPSSPP_ARCH(64BIT) && !defined(USE_ASAN) && !defined(NO_MMAP)""",
        """u8* MemArena::Find4GBBase() {
#if PPSSPP_PLATFORM(PS5)
	void *base = nullptr;
	if (ps5rt_vrange_reserve(0x100000000ULL, reinterpret_cast<void*>(0x600000000ULL), 0x10000, &base) != 0)
		return nullptr;
	return static_cast<u8*>(base);
#else
	// Now, create views in high memory where there's plenty of space.
#if PPSSPP_ARCH(64BIT) && !defined(USE_ASAN) && !defined(NO_MMAP)""",
        "MemArena Find4GBBase begin",
    )
    text = replace_once(
        text,
        """	return static_cast<u8*>(base);
#endif
}

#endif""",
        """	return static_cast<u8*>(base);
#endif
#endif
}

#endif""",
        "MemArena Find4GBBase end",
    )
    out[path] = text

    path = root / "Common/MemoryUtil.cpp"
    text = path.read_text()
    text = replace_once(
        text,
        """#ifndef _WIN32
#include <unistd.h>
#endif""",
        """#ifndef _WIN32
#include <unistd.h>
#if PPSSPP_PLATFORM(PS5)
#include <ps5rt/c/exec.h>
#endif
#endif""",
        "MemoryUtil include",
    )
    text = replace_once(
        text,
        """	void* ptr = mmap(map_hint, size, prot, MAP_ANON | MAP_PRIVATE, -1, 0);

	if (ptr == MAP_FAILED) {""",
        """#if PPSSPP_PLATFORM(PS5)
	void* ptr = ps5rt_exec_allocate(size, reinterpret_cast<uintptr_t>(map_hint));
#else
	void* ptr = mmap(map_hint, size, prot, MAP_ANON | MAP_PRIVATE, -1, 0);
#endif

	if (ptr == MAP_FAILED || ptr == nullptr) {""",
        "MemoryUtil executable allocate",
    )
    text = replace_once(
        text,
        """void FreeExecutableMemory(void *ptr, size_t size) {
	FreeMemoryPages(ptr, size);
}""",
        """void FreeExecutableMemory(void *ptr, size_t size) {
#if PPSSPP_PLATFORM(PS5)
	(void)size;
	ps5rt_exec_release(ptr);
#else
	FreeMemoryPages(ptr, size);
#endif
}""",
        "MemoryUtil executable free",
    )
    out[path] = text

    path = root / "Common/Thread/ThreadUtil.cpp"
    text = path.read_text()
    text = replace_once(
        text,
        """#elif defined(__DragonFly__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	pthread_set_name_np(pthread_self(), threadName);""",
        """#elif (defined(__DragonFly__) || defined(__FreeBSD__) || defined(__OpenBSD__)) && !PPSSPP_PLATFORM(PS5)
	pthread_set_name_np(pthread_self(), threadName);""",
        "ThreadUtil pthread naming",
    )
    out[path] = text

    return out


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    root = args.source.resolve()
    if not (root / ".git").exists():
        raise SystemExit(f"not a git checkout: {root}")

    head = subprocess.check_output(["git", "-C", str(root), "rev-parse", "HEAD"], text=True).strip()
    if head != EXPECTED:
        raise SystemExit(f"wrong PPSSPP revision: {head}; expected {EXPECTED}")

    dirty = subprocess.run(["git", "-C", str(root), "diff", "--quiet"]).returncode != 0
    if dirty:
        raise SystemExit("PPSSPP checkout has local modifications")

    try:
        transformed = transform(root)
    except RuntimeError as exc:
        raise SystemExit(str(exc))

    if args.check:
        print(f"PPSSPP {EXPECTED}: {len(transformed)} files matched all PS5 transformations")
        return 0

    for path, content in transformed.items():
        path.write_text(content)

    marker = root / ".native-emus-ps5-engine-patch"
    marker.write_text(EXPECTED + "\n")
    print(f"PPSSPP {EXPECTED}: native PS5 engine transformations applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
