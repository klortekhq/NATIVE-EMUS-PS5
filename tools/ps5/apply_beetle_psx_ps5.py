#!/usr/bin/env python3
"""Enable Beetle PSX HW Lightrec on PS5 using ps5rt executable memory.

Pinned donor:
  mihawk-99/PS5_BeetlePSX@e43b3980e031c47066917c941be6ace6f51ed24f

The donor deliberately disables Lightrec on PS5 until its code buffer can be
allocated from console executable memory. This transform changes only that
platform boundary: PSX RAM/BIOS/scratch continue using the core's normal
fallback allocations, while Lightrec's TLSF code pool is backed by ps5rt.
"""

from __future__ import annotations
import argparse
import pathlib
import subprocess

EXPECTED = "e43b3980e031c47066917c941be6ace6f51ed24f"


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected 1 match, found {count}")
    return text.replace(old, new, 1)


def transform(root: pathlib.Path) -> dict[pathlib.Path, str]:
    out: dict[pathlib.Path, str] = {}

    makefile = root / "Makefile"
    text = makefile.read_text()
    text = replace_once(
        text,
        """ifeq ($(platform), ps5)
   HAVE_VULKAN = 1
   HAVE_OPENGL = 0
   HAVE_CDROM = 0
   HAVE_LIGHTREC = 0
   LINK_STATIC_LIBCPLUSPLUS = 0
endif
""",
        """ifeq ($(platform), ps5)
   HAVE_VULKAN = 1
   HAVE_OPENGL = 0
   HAVE_CDROM = 0
   # NATIVE-EMUS-PS5: Lightrec is the required final CPU path.
   # Its code pool is supplied by ps5rt in libretro.c.
   HAVE_LIGHTREC = 1
   LINK_STATIC_LIBCPLUSPLUS = 0
endif
""",
        "PS5 Lightrec enable",
    )
    out[makefile] = text

    libretro = root / "libretro.c"
    text = libretro.read_text()

    text = replace_once(
        text,
        """#ifdef HAVE_LIGHTREC
#include <lightrec-config.h>
""",
        """#ifdef HAVE_LIGHTREC
#include <lightrec-config.h>
#if defined(__PROSPERO__)
#include <ps5rt/c/exec.h>
#endif
""",
        "ps5rt executable-memory include",
    )

    # The donor's generic lightrec_init_mmap() still references the desktop
    # SHM/memfd setup even though PS5 never calls that path after this transform.
    # Stub it on Prospero so enabling HAVE_LIGHTREC does not drag the desktop
    # mmap allocator into the PS5 compile.
    text = replace_once(
        text,
        """int lightrec_init_mmap(void)
{
\tint ret = 0;
""",
        """int lightrec_init_mmap(void)
{
#if defined(__PROSPERO__)
\treturn 0;
#else
\tint ret = 0;
""",
        "PS5 lightrec_init_mmap stub",
    )
    text = replace_once(
        text,
        """#ifdef HAVE_WIN_SHM
\tCloseHandle(memfd);
#endif
\treturn ret;
}

void lightrec_free_mmap(void)
""",
        """#ifdef HAVE_WIN_SHM
\tCloseHandle(memfd);
#endif
\treturn ret;
#endif
}

void lightrec_free_mmap(void)
""",
        "PS5 lightrec_init_mmap stub end",
    )

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
    * PS5: keep the donor's ordinary heap-backed PSX RAM path, but place the
    * Lightrec TLSF code pool in real executable console memory. Lightrec then
    * calls jit_set_code() on slices of this pool, so GNU Lightning emits
    * native x86-64 machine code directly into PS5 executable memory.
    *
    * No interpreter substitution and no Linux mmap compatibility layer.
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
    text = replace_once(text, old_init, new_init, "PS5 Lightrec code-pool init")

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
    text = replace_once(text, old_cleanup, new_cleanup, "PS5 Lightrec code-pool cleanup")

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
        raise SystemExit(f"wrong PS5_BeetlePSX revision: {head}; expected {EXPECTED}")
    if subprocess.run(["git", "-C", str(root), "diff", "--quiet"]).returncode:
        raise SystemExit("PS5_BeetlePSX checkout has local modifications")

    try:
        outputs = transform(root)
    except RuntimeError as exc:
        raise SystemExit(str(exc))

    if args.check:
        print(f"PS5_BeetlePSX {EXPECTED}: Lightrec -> ps5rt transform matched")
        return 0

    for path, content in outputs.items():
        path.write_text(content)

    (root / ".native-emus-ps5-lightrec").write_text(EXPECTED + "\n")
    print("Beetle PSX HW PS5: Lightrec executable pool retargeted to ps5rt")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
