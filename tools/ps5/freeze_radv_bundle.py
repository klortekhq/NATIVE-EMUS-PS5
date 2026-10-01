#!/usr/bin/env python3
"""Freeze an already-built PS5 RADV dependency tree into a hash receipt."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

REQUIRED_FILES = (
    "lib/libvulkan_radeon.ps5.a",
    "tools/radv-link.sh",
    "tooling/native/ps5-pie.ld",
    "tooling/psbc/ps5-pie-unwind.ld",
    "tooling/native/app_crt.cpp",
    "tooling/native/app_cpp_runtime.cpp",
    "vendor/ps5/sdk/stubs/agc_canary_link_stub.c",
    "vendor/ps5/sdk/stubs/agc_driver_canary_link_stub.c",
    "sdk/bin/prospero-clang",
    "sdk/bin/prospero-clang++",
    "sdk/bin/prospero-lld",
    "sdk/target/lib/libps5platform.a",
)

REVISIONS = {
    "ps5_vulkan": "71026e7ec1951fe72ae5b9118ff0905216a4c220",
    "ps5_mesa": "cedb774b27d089fa81f46add28d0a8c13ff0f7d2",
    "ps5_payload_sdk": "95c08f27386fc698f6bbe21dde3030140a41d10b",
}


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("bundle", type=Path)
    args = parser.parse_args()
    root = args.bundle.resolve()

    files: dict[str, str] = {}
    for relative in REQUIRED_FILES:
        path = root / relative
        if not path.is_file():
            raise SystemExit(f"cannot freeze incomplete RADV bundle: missing {relative}")
        files[relative] = digest(path)

    manifest = {
        "schema_version": 1,
        "revisions": REVISIONS,
        "files": files,
        "note": (
            "Immutable PS5 RADV native-link bundle. Hashes cover the driver, "
            "link recipe, linker scripts, native CRT/runtime, AGC link stubs and matching SDK."
        ),
    }
    (root / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"wrote {root / 'manifest.json'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
