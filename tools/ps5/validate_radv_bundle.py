#!/usr/bin/env python3
"""Validate an immutable PS5 RADV bundle used by native emulator links."""

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

EXPECTED_REVISIONS = {
    "ps5_vulkan": "71026e7ec1951fe72ae5b9118ff0905216a4c220",
    "ps5_mesa": "cedb774b27d089fa81f46add28d0a8c13ff0f7d2",
    "ps5_payload_sdk": "95c08f27386fc698f6bbe21dde3030140a41d10b",
}


def sha256(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def validate(root: Path) -> dict:
    manifest_path = root / "manifest.json"
    if not manifest_path.is_file():
        raise SystemExit(f"missing RADV bundle manifest: {manifest_path}")

    manifest = json.loads(manifest_path.read_text())
    if manifest.get("schema_version") != 1:
        raise SystemExit("unsupported RADV bundle schema")

    revisions = manifest.get("revisions", {})
    for key, expected in EXPECTED_REVISIONS.items():
        actual = revisions.get(key)
        if actual != expected:
            raise SystemExit(
                f"RADV bundle revision mismatch for {key}: {actual!r} != {expected!r}"
            )

    files = manifest.get("files", {})
    for relative in REQUIRED_FILES:
        path = root / relative
        if not path.is_file():
            raise SystemExit(f"RADV bundle missing required file: {relative}")
        expected = files.get(relative)
        if not expected:
            raise SystemExit(f"RADV bundle manifest missing digest: {relative}")
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(
                f"RADV bundle digest mismatch for {relative}: {actual} != {expected}"
            )

    archive = root / "lib/libvulkan_radeon.ps5.a"
    if archive.stat().st_size == 0:
        raise SystemExit("RADV archive is empty")

    return manifest


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("bundle", type=Path)
    args = parser.parse_args()

    root = args.bundle.resolve()
    manifest = validate(root)
    print(
        "RADV bundle PASS "
        f"mesa={manifest['revisions']['ps5_mesa'][:8]} "
        f"vulkan={manifest['revisions']['ps5_vulkan'][:8]}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
