#!/usr/bin/env python3
"""Host regression tests for the immutable PS5 RADV bundle receipt."""

from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
FREEZER = ROOT / "tools/ps5/freeze_radv_bundle.py"
VALIDATOR = ROOT / "tools/ps5/validate_radv_bundle.py"

FILES = (
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


def run(*args: str, ok: bool = True) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        [sys.executable, *args],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if ok and result.returncode:
        raise AssertionError(result.stdout)
    if not ok and result.returncode == 0:
        raise AssertionError("command unexpectedly succeeded:\n" + result.stdout)
    return result


with tempfile.TemporaryDirectory(prefix="native-emus-radv-") as tmp:
    bundle = Path(tmp)
    for index, relative in enumerate(FILES):
        path = bundle / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        # The validator intentionally checks identity, not native object syntax.
        path.write_bytes(f"fixture-{index}-{relative}\n".encode())

    freeze = run(str(FREEZER), str(bundle))
    assert "manifest.json" in freeze.stdout

    manifest_path = bundle / "manifest.json"
    manifest = json.loads(manifest_path.read_text())
    assert manifest["schema_version"] == 1
    assert set(manifest["files"]) == set(FILES)

    first_manifest = manifest_path.read_bytes()
    run(str(VALIDATOR), str(bundle))

    # Freezing the exact same bundle must be byte-for-byte deterministic.
    run(str(FREEZER), str(bundle))
    assert manifest_path.read_bytes() == first_manifest

    # Any dependency mutation must invalidate the receipt.
    driver = bundle / "lib/libvulkan_radeon.ps5.a"
    driver.write_bytes(driver.read_bytes() + b"tamper")
    failed = run(str(VALIDATOR), str(bundle), ok=False)
    assert "digest mismatch" in failed.stdout

    # Re-freeze acknowledges the new immutable input and restores validity.
    run(str(FREEZER), str(bundle))
    run(str(VALIDATOR), str(bundle))

print("PS1 RADV bundle freeze/validate/tamper regression PASS")
