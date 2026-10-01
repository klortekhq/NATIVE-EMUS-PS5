#!/usr/bin/env python3
"""Adapt upstream Beetle PSX HW to the native PS5 Lightrec/ps5rt path.

Canonical upstream:
  libretro/beetle-psx-libretro@ed87921996c67658d7a70814f73034bbca08786a

The PS5 CPU policy is intentionally strict:
  R3000A -> Lightrec -> GNU Lightning x86-64 -> PS5 Zen 2.

PSX RAM/BIOS/scratch remain on Beetle's ordinary heap-backed fallback path on
Prospero. Only the Lightrec code pool requires executable console memory and is
allocated through ps5rt. This avoids carrying desktop mmap/memfd semantics into
the PS5 title while preserving the native recompiler.
"""

from __future__ import annotations

import argparse
import pathlib
import subprocess

EXPECTED = "ed87921996c67658d7a70814f73034bbca08786a"


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected 1 match, found {count}")
    return text.replace(old, new, 1)


def wrap_lightrec_mapping_for_ps5(text: str) -> str:
    start_marker = "#ifdef HAVE_LIGHTREC\n/* Address-space strategy:"
    end_marker = "#endif /* HAVE_LIGHTREC */\n\n/* LED interface */"

    start = text.find(start_marker)
    end = text.find(end_marker, start)
    if start < 0 or end < 0:
        raise RuntimeError("Lightrec mmap section markers not found")

    outer_prefix = "#ifdef HAVE_LIGHTREC\n"
    inner_start = start + len(outer_prefix)
    inner = text[inner_start:end]

    wrapped = (
        "#ifdef HAVE_LIGHTREC\n"
        "#if defined(__PROSPERO__)\n"
        "/* PS5 uses heap-backed guest RAM and a ps5rt executable code pool. */\n"
        "int lightrec_init_mmap(void)\n"
        "{\n"
        "\treturn 0;\n"
        "}\n\n"
        "void lightrec_free_mmap(void)\n"
        "{\n"
        "\t/* No desktop mmap mappings are created on Prospero. */\n"
        "}\n"
        "#else\n"
        + inner +
        "#endif /* !__PROSPERO__ */\n"
        "#endif /* HAVE_LIGHTREC */\n\n"
        "/* LED interface */"
    )
    return text[:start] + wrapped + text[end + len(end_marker):]


def transform(root: pathlib.Path) -> dict[pathlib.Path, str]:
    out: dict[pathlib.Path, str] = {}

    makefile = root / "Makefile"
    text = makefile.read_text()

    ps5_block = """# PlayStation 5 native static engine
else ifeq ($(platform), ps5)
   TARGET := $(TARGET_NAME)_libretro_ps5.a
   fpic := -fPIC
   STATIC_LINKING = 1
   IS_X86 = 1
   HAVE_VULKAN = 1
   HAVE_OPENGL = 0
   HAVE_CDROM = 0
   HAVE_LIGHTREC = 1
   THREADED_RECOMPILER = 1
   LINK_STATIC_LIBCPLUSPLUS = 0
   NEED_THREADING = 1
   FLAGS += -D__PROSPERO__ -DHAVE_HW
   FLAGS += -march=znver2 -msse4.1 -mavx2 -mno-vzeroupper

"""
    text = replace_once(
        text,
        "# GCW0\nelse ifeq ($(platform), gcw0)\n",
        ps5_block + "# GCW0\nelse ifeq ($(platform), gcw0)\n",
        "PS5 Makefile platform block",
    )
    out[makefile] = text

    lightning_h = root / "include/lightning.h"
    text = lightning_h.read_text()
    text = replace_once(
        text,
        "#define HAVE_MMAP 1\n\n#include <lightning-actual.h>\n",
        "#if defined(__PROSPERO__)\n"
        "#define HAVE_MMAP 0 /* caller-supplied ps5rt executable code pool */\n"
        "#else\n"
        "#define HAVE_MMAP 1\n"
        "#endif\n\n"
        "#include <lightning-actual.h>\n",
        "lightning mmap policy",
    )
    out[lightning_h] = text

    libretro = root / "libretro.c"
    text = libretro.read_text()

    old_include = """#ifdef HAVE_LIGHTREC
#include <lightrec-config.h>
#define _GNU_SOURCE /* For MFD_HUGETLB feature test macro define */
#include <sys/mman.h>

#ifdef HAVE_ASHMEM
#include <sys/ioctl.h>
#include <linux/ashmem.h>
#include <dlfcn.h>
#endif

#if defined(HAVE_SHM) || defined(HAVE_ASHMEM)
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#ifdef HAVE_WIN_SHM
#include <windows.h>
#endif
#endif /* HAVE_LIGHTREC */
"""
    new_include = """#ifdef HAVE_LIGHTREC
#include <lightrec-config.h>
#if defined(__PROSPERO__)
#include <ps5rt/c/exec.h>
#else
#define _GNU_SOURCE /* For MFD_HUGETLB feature test macro define */
#include <sys/mman.h>

#ifdef HAVE_ASHMEM
#include <sys/ioctl.h>
#include <linux/ashmem.h>
#include <dlfcn.h>
#endif

#if defined(HAVE_SHM) || defined(HAVE_ASHMEM)
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#ifdef HAVE_WIN_SHM
#include <windows.h>
#endif
#endif /* __PROSPERO__ */
#endif /* HAVE_LIGHTREC */
"""
    text = replace_once(
        text, old_include, new_include,
        "PS5 Lightrec executable-memory include",
    )

    text = wrap_lightrec_mapping_for_ps5(text)

    old_init = """#ifdef HAVE_LIGHTREC
   /* try hugetlb then fallback if mmap fails */
   hugetlb = true;
   psx_mmap = lightrec_init_mmap();

   if(psx_mmap == 0)
   {
      hugetlb = false;
      psx_mmap = lightrec_init_mmap();
   }

   if(psx_mmap > 0)
   {
      MainRAM = MultiAccessSizeMem_Attach(psx_mem, RAM_SIZE);
      ScratchRAM = MultiAccessSizeMem_Attach(psx_scratch, SCRATCH_SIZE);
      BIOSROM = MultiAccessSizeMem_Attach(psx_bios, BIOS_SIZE);
   }
   else
#endif
   {
      MainRAM = MultiAccessSizeMem_New(RAM_SIZE);
      ScratchRAM = MultiAccessSizeMem_New(SCRATCH_SIZE);
      BIOSROM = MultiAccessSizeMem_New(BIOS_SIZE);
   }
"""
    new_init = """#ifdef HAVE_LIGHTREC
#if defined(__PROSPERO__)
   /*
    * Native PS5 path:
    *   - guest RAM/BIOS/scratch stay heap-backed;
    *   - the Lightrec TLSF code pool is real executable console memory;
    *   - GNU Lightning emits x86-64 directly for Zen 2.
    */
   psx_mmap = 0;
   if (!lightrec_codebuffer)
      lightrec_codebuffer = (uint8_t *)ps5rt_exec_allocate(
            LIGHTREC_CODEBUFFER_SIZE, 0);

   if (!lightrec_codebuffer)
   {
      log_cb(RETRO_LOG_ERROR,
            "PS5 Lightrec: executable code-buffer allocation failed (%u bytes)\\n",
            (unsigned)LIGHTREC_CODEBUFFER_SIZE);
      return;
   }
#else
   /* try hugetlb then fallback if mmap fails */
   hugetlb = true;
   psx_mmap = lightrec_init_mmap();

   if(psx_mmap == 0)
   {
      hugetlb = false;
      psx_mmap = lightrec_init_mmap();
   }

   if(psx_mmap > 0)
   {
      MainRAM = MultiAccessSizeMem_Attach(psx_mem, RAM_SIZE);
      ScratchRAM = MultiAccessSizeMem_Attach(psx_scratch, SCRATCH_SIZE);
      BIOSROM = MultiAccessSizeMem_Attach(psx_bios, BIOS_SIZE);
   }
   else
#endif
#endif
   {
      MainRAM = MultiAccessSizeMem_New(RAM_SIZE);
      ScratchRAM = MultiAccessSizeMem_New(SCRATCH_SIZE);
      BIOSROM = MultiAccessSizeMem_New(BIOS_SIZE);
   }
"""
    text = replace_once(
        text, old_init, new_init,
        "PS5 Lightrec code-pool initialization",
    )

    old_cleanup = """   MainRAM    = NULL;
   ScratchRAM = NULL;
   BIOSROM    = NULL;
   if(lightrec_codebuffer)
      lightrec_codebuffer = NULL;
#else
"""
    new_cleanup = """   MainRAM    = NULL;
   ScratchRAM = NULL;
   BIOSROM    = NULL;
#if defined(__PROSPERO__)
   if (lightrec_codebuffer)
   {
      ps5rt_exec_release(lightrec_codebuffer);
      lightrec_codebuffer = NULL;
   }
#else
   if(lightrec_codebuffer)
      lightrec_codebuffer = NULL;
#endif
#else
"""
    text = replace_once(
        text, old_cleanup, new_cleanup,
        "PS5 Lightrec code-pool cleanup",
    )

    out[libretro] = text
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
        raise SystemExit(
            f"wrong Beetle PSX revision: {head}; expected {EXPECTED}"
        )
    if subprocess.run(
        ["git", "-C", str(root), "diff", "--quiet"]
    ).returncode:
        raise SystemExit("Beetle PSX checkout has local modifications")

    try:
        outputs = transform(root)
    except RuntimeError as exc:
        raise SystemExit(str(exc))

    if args.check:
        print(
            f"Beetle PSX {EXPECTED}: upstream -> native PS5 Lightrec "
            f"transform matched ({len(outputs)} files)"
        )
        return 0

    for path, content in outputs.items():
        path.write_text(content)

    (root / ".native-emus-ps5-lightrec").write_text(EXPECTED + "\n")
    print(
        "Beetle PSX upstream: native PS5 Lightrec executable pool "
        "retargeted to ps5rt"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
