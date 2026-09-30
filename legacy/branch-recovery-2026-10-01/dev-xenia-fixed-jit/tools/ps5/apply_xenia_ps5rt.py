#!/usr/bin/env python3
"""Retarget Xenia's fixed x64 generated-code cache to ps5rt."""

from __future__ import annotations

import argparse
import pathlib
import subprocess

EXPECTED = "95a5c3ee250f80c3b9d139658649d9ffb6db3eec"
HEADER = pathlib.Path("src/xenia/cpu/backend/x64/x64_code_cache.h")
SOURCE = pathlib.Path("src/xenia/cpu/backend/x64/x64_code_cache.cc")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one exact match, found {count}")
    return text.replace(old, new, 1)


def transform_header(text: str) -> str:
    text = replace_once(
        text,
        '#include "xenia/cpu/backend/code_cache.h"\n',
        '#include "xenia/cpu/backend/code_cache.h"\n'
        '#ifdef __PROSPERO__\n'
        '#include <ps5rt/jit.hpp>\n'
        '#endif\n',
        "ps5rt JIT include",
    )
    text = replace_once(
        text,
        """  xe::memory::FileMappingHandle mapping_ =
      xe::memory::kFileMappingHandleInvalid;
""",
        """  xe::memory::FileMappingHandle mapping_ =
      xe::memory::kFileMappingHandleInvalid;
#ifdef __PROSPERO__
  ps5rt::JitRegion ps5_generated_code_jit_{};
#endif
""",
        "PS5 JIT lifetime member",
    )
    return text


def transform_source(text: str) -> str:
    text = replace_once(
        text,
        """  // Unmap all views and close mapping.
  if (mapping_ != xe::memory::kFileMappingHandleInvalid) {
""",
        """  // Unmap all views and close mapping.
#ifdef __PROSPERO__
  if (ps5_generated_code_jit_) {
    (void)ps5rt::destroy_jit_region(ps5_generated_code_jit_);
  }
#else
  if (mapping_ != xe::memory::kFileMappingHandleInvalid) {
""",
        "PS5 JIT destruction",
    )
    text = replace_once(
        text,
        """    mapping_ = xe::memory::kFileMappingHandleInvalid;
  }
}

bool X64CodeCache::Initialize() {
""",
        """    mapping_ = xe::memory::kFileMappingHandleInvalid;
  }
#endif
}

bool X64CodeCache::Initialize() {
""",
        "PS5 destructor conditional close",
    )

    begin = """  // Create mmap file. This allows us to share the code cache with the debugger.
  file_name_ = fmt::format("xenia_code_cache_{}", Clock::QueryHostTickCount());
  mapping_ = xe::memory::CreateFileMappingHandle(
      file_name_, kGeneratedCodeSize, xe::memory::PageAccess::kExecuteReadWrite,
      false);
"""
    replacement = """#ifdef __PROSPERO__
  // Xenia's generated-code ABI relies on two fixed low-address aliases:
  // RX at 0xA0000000 and RW at 0xB0000000, backed by the same memory.
  ps5rt::JitRequest ps5_jit{};
  ps5_jit.size = kGeneratedCodeSize + 1;
  ps5_jit.alignment = 0x4000;
  ps5_jit.debug_name = "xenia-x64-code-cache";
  ps5_jit.prefer_dual_mapping = true;
  ps5_jit.preferred_execute_address =
      reinterpret_cast<void*>(kGeneratedCodeExecuteBase);
  ps5_jit.preferred_write_address =
      reinterpret_cast<void*>(kGeneratedCodeWriteBase);
  ps5_jit.require_fixed_execute = true;
  ps5_jit.require_fixed_write = true;

  if (!ps5rt::create_jit_region(ps5_jit, ps5_generated_code_jit_)) {
    XELOGE("Unable to allocate fixed PS5 x64 JIT code cache");
    return false;
  }

  generated_code_execute_base_ =
      static_cast<uint8_t*>(ps5_generated_code_jit_.execute_view.address);
  generated_code_write_base_ =
      static_cast<uint8_t*>(ps5_generated_code_jit_.write_view.address);

  // ps5rt maps the full direct-memory backing up front, so Xenia's later
  // high-water commit calls must become no-ops.
  generated_code_commit_mark_.store(kGeneratedCodeSize);
#else
  // Create mmap file. This allows us to share the code cache with the debugger.
  file_name_ = fmt::format("xenia_code_cache_{}", Clock::QueryHostTickCount());
  mapping_ = xe::memory::CreateFileMappingHandle(
      file_name_, kGeneratedCodeSize, xe::memory::PageAccess::kExecuteReadWrite,
      false);
"""
    text = replace_once(text, begin, replacement, "PS5 fixed code cache start")

    end = """      return false;
    }
  }

  // Preallocate the function map to a large, reasonable size.
"""
    end_repl = """      return false;
    }
  }
#endif

  // Preallocate the function map to a large, reasonable size.
"""
    text = replace_once(text, end, end_repl, "PS5 fixed code cache end")
    return text


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
        raise SystemExit(f"wrong Xenia revision: {head}; expected {EXPECTED}")
    if subprocess.run(["git", "-C", str(root), "diff", "--quiet"]).returncode:
        raise SystemExit("Xenia checkout has local modifications")

    try:
        header = transform_header((root / HEADER).read_text())
        source = transform_source((root / SOURCE).read_text())
    except RuntimeError as exc:
        raise SystemExit(str(exc))

    for needle in (
        "kGeneratedCodeExecuteBase = 0xA0000000",
        "kGeneratedCodeWriteBase =",
    ):
        if needle not in header:
            raise SystemExit(f"Xenia fixed-address ABI changed: missing {needle}")

    if args.check:
        if "require_fixed_execute = true" not in source:
            raise SystemExit("PS5 fixed execute mapping missing")
        if "require_fixed_write = true" not in source:
            raise SystemExit("PS5 fixed write mapping missing")
        print(f"Xenia {EXPECTED}: fixed x64 JIT transform matched")
        return 0

    (root / HEADER).write_text(header)
    (root / SOURCE).write_text(source)
    (root / ".native-emus-ps5-xenia").write_text(EXPECTED + "\n")
    print("Xenia fixed x64 generated-code cache retargeted to ps5rt")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
