#!/usr/bin/env python3
"""Offline regression tests for transforms whose historical PS5 donors may vanish.

These tests do not replace exact-source integration when a pinned donor is
publicly reachable. They preserve our own deterministic transformation contract
so CI still detects accidental ps5rt/JIT regressions while external provenance
is temporarily unavailable.
"""

from __future__ import annotations

import argparse
import importlib.util
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]


def load(name: str, relative: str):
    path = ROOT / relative
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_dynarmic() -> None:
    module = load("apply_dynarmic_ps5rt", "tools/ps5/apply_dynarmic_ps5rt.py")
    source = """#if defined(__PROSPERO__)
#    include <ps5platform/exec.h>
void* p = ps5_exec_allocate(size, reinterpret_cast<uintptr_t>(this));
ps5_exec_release(p);
#endif
"""
    out = module.transform(source)
    assert "#    include <ps5rt/c/exec.h>" in out
    assert "ps5rt_exec_allocate(size, reinterpret_cast<uintptr_t>(this))" in out
    assert "ps5rt_exec_release(p)" in out
    assert "ps5platform/exec.h" not in out
    assert "ps5_exec_allocate(" not in out
    assert "ps5_exec_release(" not in out


def test_mupen() -> None:
    module = load("apply_mupen64plus_ps5rt", "tools/ps5/apply_mupen64plus_ps5rt.py")
    source = """#include <ps5platform/exec.h>
void* code = ps5_exec_allocate(size, 0);
ps5_exec_release(code);
"""
    out = module.transform(source)
    assert "#include <ps5rt/c/exec.h>" in out
    assert "ps5rt_exec_allocate(size, 0)" in out
    assert "ps5rt_exec_release(code)" in out
    assert "ps5platform/exec.h" not in out
    assert "ps5_exec_allocate(" not in out
    assert "ps5_exec_release(" not in out

    makefile = """ifeq ($(platform), ps5)
   TARGET := $(TARGET_NAME)_libretro.so
   FLAGS += -D__PROSPERO__
endif
"""
    made = module.transform_makefile(makefile)
    assert "ifeq ($(STATIC_LINKING), 1)" in made
    assert "TARGET := $(TARGET_NAME)_libretro.a" in made
    assert "TARGET := $(TARGET_NAME)_libretro.so" in made


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("target", choices=("dynarmic", "mupen", "all"), nargs="?", default="all")
    args = parser.parse_args()

    if args.target in ("dynarmic", "all"):
        test_dynarmic()
        print("offline PS5 Dynarmic -> ps5rt transform contract: PASS")
    if args.target in ("mupen", "all"):
        test_mupen()
        print("offline PS5 Mupen64Plus -> ps5rt transform contract: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
