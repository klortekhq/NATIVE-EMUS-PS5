#!/usr/bin/env python3
"""Retarget Azahar's pinned Dynarmic x64 code cache to ps5rt sparse arenas."""

from __future__ import annotations

import argparse
import pathlib
import subprocess

EXPECTED = "e77b1ba0b7da7cbe93021b01a663acfe7c4dd516"
CPP = pathlib.Path("src/dynarmic/backend/x64/block_of_code.cpp")
HPP = pathlib.Path("src/dynarmic/backend/x64/block_of_code.h")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one exact match, found {count}")
    return text.replace(old, new, 1)


def transform_cpp(text: str) -> str:
    text = replace_once(
        text,
        """#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#else
#    include <sys/mman.h>
#endif
""",
        """#ifdef __PROSPERO__
#    include <ps5rt/c/sparse_arena.h>
#elif defined(_WIN32)
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#else
#    include <sys/mman.h>
#endif
""",
        "platform includes",
    )

    text = replace_once(
        text,
        """#include <array>
#include <cstring>
""",
        """#include <array>
#include <cstring>
#include <mutex>
#include <unordered_map>
""",
        "standard includes",
    )

    marker = """constexpr size_t CONSTANT_POOL_SIZE = 2 * 1024 * 1024;
constexpr size_t PRELUDE_COMMIT_SIZE = 16 * 1024 * 1024;

"""
    injected = marker + """#ifdef __PROSPERO__
constexpr size_t PS5_PAGE_SIZE = 0x4000;

std::mutex g_ps5_arenas_mutex;
std::unordered_map<uint8_t*, ps5rt_sparse_arena> g_ps5_arenas;

size_t RoundPs5Page(size_t size) {
    return (size + PS5_PAGE_SIZE - 1) & ~(PS5_PAGE_SIZE - 1);
}

int CommitPs5Arena(const void* base, size_t offset, size_t size) {
    std::scoped_lock lock(g_ps5_arenas_mutex);
    auto it = g_ps5_arenas.find(
        const_cast<uint8_t*>(static_cast<const uint8_t*>(base)));
    if (it == g_ps5_arenas.end()) {
        return -1;
    }
    return ps5rt_sparse_arena_commit(
        &it->second, offset, size,
        PS5RT_SPARSE_READ | PS5RT_SPARSE_WRITE);
}

int ProtectPs5Arena(const void* base, size_t size, bool executable) {
    std::scoped_lock lock(g_ps5_arenas_mutex);
    auto it = g_ps5_arenas.find(
        const_cast<uint8_t*>(static_cast<const uint8_t*>(base)));
    if (it == g_ps5_arenas.end()) {
        return -1;
    }
    const unsigned flags = executable
        ? (PS5RT_SPARSE_READ | PS5RT_SPARSE_EXEC)
        : (PS5RT_SPARSE_READ | PS5RT_SPARSE_WRITE);
    return ps5rt_sparse_arena_protect(&it->second, 0, size, flags);
}
#endif

"""
    text = replace_once(text, marker, injected, "PS5 arena registry")

    text = replace_once(
        text,
        """class CustomXbyakAllocator : public Xbyak::Allocator {
public:
#ifdef _WIN32
""",
        """class CustomXbyakAllocator : public Xbyak::Allocator {
public:
#ifdef __PROSPERO__
    uint8_t* alloc(size_t size) override {
        ps5rt_sparse_arena arena{};
        if (ps5rt_sparse_arena_create(size, nullptr, 0x200000, &arena) != 0 ||
            arena.base == nullptr) {
            throw Xbyak::Error(Xbyak::ERR_CANT_ALLOC);
        }

        auto* base = static_cast<uint8_t*>(arena.base);
        std::scoped_lock lock(g_ps5_arenas_mutex);
        auto [it, inserted] = g_ps5_arenas.emplace(base, arena);
        if (!inserted) {
            ps5rt_sparse_arena_destroy(&arena);
            throw Xbyak::Error(Xbyak::ERR_CANT_ALLOC);
        }
        return base;
    }

    void free(uint8_t* p) override {
        ps5rt_sparse_arena arena{};
        {
            std::scoped_lock lock(g_ps5_arenas_mutex);
            auto it = g_ps5_arenas.find(p);
            if (it == g_ps5_arenas.end()) {
                return;
            }
            arena = it->second;
            g_ps5_arenas.erase(it);
        }
        ps5rt_sparse_arena_destroy(&arena);
    }

    bool useProtect() const override { return false; }
#elif defined(_WIN32)
""",
        "PS5 allocator",
    )

    text = replace_once(
        text,
        """void ProtectMemory(const void* base, size_t size, bool is_executable) {
#    ifdef _WIN32
    DWORD oldProtect = 0;
    VirtualProtect(const_cast<void*>(base), size, is_executable ? PAGE_EXECUTE_READ : PAGE_READWRITE, &oldProtect);
#    else
""",
        """void ProtectMemory(const void* base, size_t size, bool is_executable) {
#    ifdef __PROSPERO__
    if (size != 0 && ProtectPs5Arena(base, size, is_executable) != 0) {
        throw Xbyak::Error(Xbyak::ERR_CANT_PROTECT);
    }
#    elif defined(_WIN32)
    DWORD oldProtect = 0;
    VirtualProtect(const_cast<void*>(base), size, is_executable ? PAGE_EXECUTE_READ : PAGE_READWRITE, &oldProtect);
#    else
""",
        "PS5 W^X",
    )

    text = replace_once(
        text,
        """void BlockOfCode::EnsureMemoryCommitted([[maybe_unused]] size_t codesize) {
#ifdef _WIN32
    if (committed_size < size_ + codesize) {
        committed_size = std::min<size_t>(maxSize_, committed_size + codesize);
#    ifdef DYNARMIC_ENABLE_NO_EXECUTE_SUPPORT
        VirtualAlloc(top_, committed_size, MEM_COMMIT, PAGE_READWRITE);
#    else
        VirtualAlloc(top_, committed_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#    endif
    }
#endif
}
""",
        """void BlockOfCode::EnsureMemoryCommitted([[maybe_unused]] size_t codesize) {
#ifdef __PROSPERO__
    const size_t required = std::min<size_t>(maxSize_, size_ + codesize);
    const size_t target = RoundPs5Page(required);
    if (committed_size < target) {
        const size_t delta = target - committed_size;
        if (CommitPs5Arena(top_, committed_size, delta) != 0) {
            throw Xbyak::Error(Xbyak::ERR_CANT_ALLOC);
        }
        committed_size = target;
    }
#elif defined(_WIN32)
    if (committed_size < size_ + codesize) {
        committed_size = std::min<size_t>(maxSize_, committed_size + codesize);
#    ifdef DYNARMIC_ENABLE_NO_EXECUTE_SUPPORT
        VirtualAlloc(top_, committed_size, MEM_COMMIT, PAGE_READWRITE);
#    else
        VirtualAlloc(top_, committed_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#    endif
    }
#endif
}
""",
        "PS5 incremental commit",
    )
    return text


def transform_hpp(text: str) -> str:
    return replace_once(
        text,
        """#ifdef _WIN32
    size_t committed_size = 0;
#endif
""",
        """#if defined(_WIN32) || defined(__PROSPERO__)
    size_t committed_size = 0;
#endif
""",
        "committed-size state",
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    root = args.source.resolve()
    head = subprocess.check_output(
        ["git", "-C", str(root), "rev-parse", "HEAD"], text=True
    ).strip()
    if head != EXPECTED:
        raise SystemExit(f"wrong Azahar Dynarmic revision: {head}; expected {EXPECTED}")

    if subprocess.run(["git", "-C", str(root), "diff", "--quiet"]).returncode:
        raise SystemExit("Azahar Dynarmic checkout has local modifications")

    try:
        cpp = transform_cpp((root / CPP).read_text())
        hpp = transform_hpp((root / HPP).read_text())
    except RuntimeError as exc:
        raise SystemExit(str(exc))

    if args.check:
        print(f"Azahar Dynarmic {EXPECTED}: sparse PS5 allocator transform matched")
        return 0

    (root / CPP).write_text(cpp)
    (root / HPP).write_text(hpp)
    (root / ".native-emus-ps5-azahar-dynarmic").write_text(EXPECTED + "\n")
    print("Azahar Dynarmic x64 allocator retargeted to ps5rt sparse arenas")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
