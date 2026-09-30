#!/usr/bin/env python3
"""Retarget Xenia's fixed x64 generated-code cache to ps5rt."""

from __future__ import annotations
import argparse, pathlib, subprocess

EXPECTED="95a5c3ee250f80c3b9d139658649d9ffb6db3eec"
HEADER=pathlib.Path("src/xenia/cpu/backend/x64/x64_code_cache.h")
SOURCE=pathlib.Path("src/xenia/cpu/backend/x64/x64_code_cache.cc")
PLATFORM=pathlib.Path("src/xenia/base/platform.h")
BACKEND=pathlib.Path("src/xenia/cpu/backend/x64/x64_backend.cc")

def replace_once(text:str,old:str,new:str,label:str)->str:
    count=text.count(old)
    if count!=1:
        raise RuntimeError(f"{label}: expected one exact match, found {count}")
    return text.replace(old,new,1)

def transform_platform(text:str)->str:
    return replace_once(
        text,
        """#elif defined(__ANDROID__)
#define XE_PLATFORM_ANDROID 1
#define XE_PLATFORM_LINUX 1
#elif defined(__gnu_linux__)
""",
        """#elif defined(__ANDROID__)
#define XE_PLATFORM_ANDROID 1
#define XE_PLATFORM_LINUX 1
#elif defined(__PROSPERO__)
#define XE_PLATFORM_PS5 1
#elif defined(__gnu_linux__)
""",
        "PS5 platform identity",
    )


def transform_backend(text:str)->str:
    count=text.count("#if XE_PLATFORM_LINUX\n")
    if count != 2:
        raise RuntimeError(f"SysV thunk guards: expected 2 Linux guards, found {count}")
    return text.replace(
        "#if XE_PLATFORM_LINUX\n",
        "#if XE_PLATFORM_LINUX || XE_PLATFORM_PS5\n",
    )


def transform_header(text:str)->str:
    text=replace_once(
        text,
        '#include "xenia/cpu/backend/code_cache.h"\n',
        '#include "xenia/cpu/backend/code_cache.h"\n#ifdef __PROSPERO__\n#include <ps5rt/jit.hpp>\n#include <ps5rt/c/vmem.h>\n#endif\n',
        "ps5rt JIT include")
    text=replace_once(
        text,
        "  xe::memory::FileMappingHandle mapping_ =\n      xe::memory::kFileMappingHandleInvalid;\n",
        "  xe::memory::FileMappingHandle mapping_ =\n      xe::memory::kFileMappingHandleInvalid;\n#ifdef __PROSPERO__\n  ps5rt::JitRegion ps5_generated_code_jit_{};\n#endif\n",
        "PS5 JIT lifetime member")
    return text

def transform_source(text:str)->str:
    text=replace_once(
        text,
        """  if (indirection_table_base_) {
    xe::memory::DeallocFixed(indirection_table_base_, 0,
                             xe::memory::DeallocationType::kRelease);
  }
""",
        """  if (indirection_table_base_) {
#ifdef __PROSPERO__
    (void)ps5rt_vrange_release(indirection_table_base_, kIndirectionTableSize);
#else
    xe::memory::DeallocFixed(indirection_table_base_, 0,
                             xe::memory::DeallocationType::kRelease);
#endif
  }
""",
        "PS5 fixed indirection destruction")

    text=replace_once(
        text,
        "  // Unmap all views and close mapping.\n  if (mapping_ != xe::memory::kFileMappingHandleInvalid) {\n",
        "  // Unmap all views and close mapping.\n#ifdef __PROSPERO__\n  if (ps5_generated_code_jit_) {\n    (void)ps5rt::destroy_jit_region(ps5_generated_code_jit_);\n  }\n#else\n  if (mapping_ != xe::memory::kFileMappingHandleInvalid) {\n",
        "PS5 JIT destruction")
    text=replace_once(
        text,
        "    mapping_ = xe::memory::kFileMappingHandleInvalid;\n  }\n}\n\nbool X64CodeCache::Initialize() {\n",
        "    mapping_ = xe::memory::kFileMappingHandleInvalid;\n  }\n#endif\n}\n\nbool X64CodeCache::Initialize() {\n",
        "PS5 destructor conditional close")

    text=replace_once(
        text,
        """  indirection_table_base_ = reinterpret_cast<uint8_t*>(xe::memory::AllocFixed(
      reinterpret_cast<void*>(kIndirectionTableBase), kIndirectionTableSize,
      xe::memory::AllocationType::kReserve,
      xe::memory::PageAccess::kReadWrite));
""",
        """#ifdef __PROSPERO__
  {
    void* ps5_indirection = nullptr;
    if (ps5rt_vrange_reserve_fixed(
            kIndirectionTableSize,
            reinterpret_cast<void*>(kIndirectionTableBase),
            0x4000, &ps5_indirection) == 0) {
      indirection_table_base_ = static_cast<uint8_t*>(ps5_indirection);
    }
  }
#else
  indirection_table_base_ = reinterpret_cast<uint8_t*>(xe::memory::AllocFixed(
      reinterpret_cast<void*>(kIndirectionTableBase), kIndirectionTableSize,
      xe::memory::AllocationType::kReserve,
      xe::memory::PageAccess::kReadWrite));
#endif
""",
        "PS5 fixed indirection reservation")

    begin="""  // Create mmap file. This allows us to share the code cache with the debugger.
  file_name_ = fmt::format("xenia_code_cache_{}", Clock::QueryHostTickCount());
  mapping_ = xe::memory::CreateFileMappingHandle(
      file_name_, kGeneratedCodeSize, xe::memory::PageAccess::kExecuteReadWrite,
      false);
"""
    replacement="""#ifdef __PROSPERO__
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
  generated_code_commit_mark_.store(kGeneratedCodeSize);
#else
  // Create mmap file. This allows us to share the code cache with the debugger.
  file_name_ = fmt::format("xenia_code_cache_{}", Clock::QueryHostTickCount());
  mapping_ = xe::memory::CreateFileMappingHandle(
      file_name_, kGeneratedCodeSize, xe::memory::PageAccess::kExecuteReadWrite,
      false);
"""
    text=replace_once(text,begin,replacement,"PS5 fixed code cache start")

    end="""      return false;
    }
  }

  // Preallocate the function map to a large, reasonable size.
"""
    end_repl="""      return false;
    }
  }
#endif

  // Preallocate the function map to a large, reasonable size.
"""
    text=replace_once(text,end,end_repl,"PS5 fixed code cache end")

    text=replace_once(
        text,
        """  // Commit the memory.
  xe::memory::AllocFixed(
      indirection_table_base_ + (guest_low - kIndirectionTableBase),
      guest_high - guest_low, xe::memory::AllocationType::kCommit,
      xe::memory::PageAccess::kReadWrite);
""",
        """  // Commit the memory.
#ifdef __PROSPERO__
  (void)ps5rt_vmem_commit(
      indirection_table_base_ + (guest_low - kIndirectionTableBase),
      guest_high - guest_low,
      PS5RT_VMEM_READ | PS5RT_VMEM_WRITE);
#else
  xe::memory::AllocFixed(
      indirection_table_base_ + (guest_low - kIndirectionTableBase),
      guest_high - guest_low, xe::memory::AllocationType::kCommit,
      xe::memory::PageAccess::kReadWrite);
#endif
""",
        "PS5 indirection commit")
    return text

def main()->int:
    ap=argparse.ArgumentParser()
    ap.add_argument("source",type=pathlib.Path)
    ap.add_argument("--check",action="store_true")
    a=ap.parse_args()
    root=a.source.resolve()
    head=subprocess.check_output(["git","-C",str(root),"rev-parse","HEAD"],text=True).strip()
    if head!=EXPECTED:
        raise SystemExit(f"wrong Xenia revision: {head}; expected {EXPECTED}")
    if subprocess.run(["git","-C",str(root),"diff","--quiet"]).returncode:
        raise SystemExit("Xenia checkout has local modifications")
    try:
        p=transform_platform((root/PLATFORM).read_text())
        b=transform_backend((root/BACKEND).read_text())
        h=transform_header((root/HEADER).read_text())
        s=transform_source((root/SOURCE).read_text())
    except RuntimeError as exc:
        raise SystemExit(str(exc))

    for needle in ("kGeneratedCodeExecuteBase = 0xA0000000","kGeneratedCodeWriteBase ="):
        if needle not in h:
            raise SystemExit(f"Xenia fixed-address ABI changed: missing {needle}")

    if a.check:
        if "#define XE_PLATFORM_PS5 1" not in p:
            raise SystemExit("Xenia PS5 platform identity missing")
        if "#if XE_PLATFORM_LINUX || XE_PLATFORM_PS5" not in b:
            raise SystemExit("Xenia PS5 SysV thunk guard missing")
        if "require_fixed_execute = true" not in s or "require_fixed_write = true" not in s:
            raise SystemExit("PS5 fixed JIT mapping missing")
        print(f"Xenia {EXPECTED}: fixed x64 JIT transform matched")
        return 0

    (root/PLATFORM).write_text(p)
    (root/BACKEND).write_text(b)
    (root/HEADER).write_text(h)
    (root/SOURCE).write_text(s)
    (root/".native-emus-ps5-xenia").write_text(EXPECTED+"\n")
    print("Xenia fixed x64 generated-code cache retargeted to ps5rt")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
