#!/usr/bin/env python3
"""Apply the minimal SCE x86-64 alignment shim required by RPCS3's LLVM pin.

The canonical LLVM source is left untouched outside two ABI-sensitive template
layouts.  The transform is deliberately marker-based and idempotent so source
drift fails loudly instead of silently patching the wrong revision.

Why this exists:
- Prospero/SCE treats the over-aligned empty SmallVectorStorage<T, 0> base as
  only pointer-aligned in the final SmallVector object.
- The same ABI loses the TrailingObjectsImpl empty-base alignment in the
  derived TrailingObjects owner.

Both failures are proven by the PS5 compile-time ABI probe in rpcs3-gate.yml.
"""

from __future__ import annotations

import argparse
from pathlib import Path

SMALL_REL = Path("llvm/include/llvm/ADT/SmallVector.h")
TRAIL_REL = Path("llvm/include/llvm/Support/TrailingObjects.h")

SMALL_OLD = """template <typename T,
          unsigned N = CalculateSmallVectorDefaultInlinedElements<T>::value>
class LLVM_GSL_OWNER SmallVector : public SmallVectorImpl<T>,
                                   SmallVectorStorage<T, N> {
"""

SMALL_NEW = """template <typename T,
          unsigned N = CalculateSmallVectorDefaultInlinedElements<T>::value>
#if defined(__SCE__)
#define LLVM_SMALLVECTOR_ALIGNAS(Ty) alignas(Ty)
#else
#define LLVM_SMALLVECTOR_ALIGNAS(Ty)
#endif
class LLVM_SMALLVECTOR_ALIGNAS(T) LLVM_GSL_OWNER SmallVector
    : public SmallVectorImpl<T>, SmallVectorStorage<T, N> {
"""

SMALL_END_OLD = """  SmallVector &operator=(std::initializer_list<T> IL) {
    this->assign(IL);
    return *this;
  }
};

template <typename T, unsigned N>
inline size_t capacity_in_bytes"""
SMALL_END_NEW = """  SmallVector &operator=(std::initializer_list<T> IL) {
    this->assign(IL);
    return *this;
  }
};

#undef LLVM_SMALLVECTOR_ALIGNAS

template <typename T, unsigned N>
inline size_t capacity_in_bytes"""

TRAIL_OLD = """class alignas(Align) TrailingObjectsImpl<Align, BaseTy, TopTrailingObj, PrevTy>
    : public TrailingObjectsBase {
protected:
"""
TRAIL_NEW = """class alignas(Align) TrailingObjectsImpl<Align, BaseTy, TopTrailingObj, PrevTy>
    : public TrailingObjectsBase {
#if defined(__SCE__)
  // The SCE x86-64 ABI may discard over-alignment carried only by an empty
  // base.  Keep a zero-length, explicitly aligned member so the base is no
  // longer represented solely through EBO while preserving its payload size.
  alignas(Align) char AlignTrailingObjects[0];
#endif

protected:
"""


def replace_once(text: str, old: str, new: str, label: str) -> tuple[str, bool]:
    if new in text:
        return text, False
    if text.count(old) != 1:
        raise SystemExit(f"{label}: expected exactly one canonical source marker")
    return text.replace(old, new, 1), True


def transform(root: Path) -> int:
    small_path = root / SMALL_REL
    trail_path = root / TRAIL_REL
    if not small_path.is_file() or not trail_path.is_file():
        raise SystemExit("LLVM source root does not contain expected headers")

    small = small_path.read_text()
    trail = trail_path.read_text()

    changed = 0
    small, did = replace_once(
        small, SMALL_OLD, SMALL_NEW, "SmallVector class alignment"
    )
    changed += did
    small, did = replace_once(
        small, SMALL_END_OLD, SMALL_END_NEW, "SmallVector macro cleanup"
    )
    changed += did
    trail, did = replace_once(
        trail, TRAIL_OLD, TRAIL_NEW, "TrailingObjects SCE alignment"
    )
    changed += did

    small_path.write_text(small)
    trail_path.write_text(trail)
    return changed


def verify(root: Path) -> None:
    small = (root / SMALL_REL).read_text()
    trail = (root / TRAIL_REL).read_text()

    required = {
        "SmallVector SCE guard": "#if defined(__SCE__)",
        "SmallVector explicit alignment": "LLVM_SMALLVECTOR_ALIGNAS(T)",
        "SmallVector cleanup": "#undef LLVM_SMALLVECTOR_ALIGNAS",
    }
    for label, needle in required.items():
        if needle not in small:
            raise SystemExit(f"{label}: missing {needle!r}")

    if "AlignTrailingObjects[0]" not in trail:
        raise SystemExit("TrailingObjects SCE aligned-member shim missing")
    if "alignas(Align) char AlignTrailingObjects[0]" not in trail:
        raise SystemExit("TrailingObjects shim lost explicit alignment")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("llvm_root", type=Path)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    root = args.llvm_root.resolve()
    if not args.check:
        changed = transform(root)
        print(f"LLVM SCE alignment transform: changed={changed}")

    verify(root)
    print("LLVM SCE alignment contract: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
