#!/usr/bin/env python3
from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
PREP = ROOT / "tools/ps5/prepare_radv_source_stack.sh"

PINS = {
    "vulkan": "71026e7ec1951fe72ae5b9118ff0905216a4c220",
    "mesa": "cedb774b27d089fa81f46add28d0a8c13ff0f7d2",
    "sdk": "95c08f27386fc698f6bbe21dde3030140a41d10b",
}

with tempfile.TemporaryDirectory() as td:
    base = Path(td)
    sources = {}
    for name, pin in PINS.items():
        src = base / f"src-{name}"
        src.mkdir()
        (src / ".native-emus-pin").write_text(pin + "\n")
        (src / "README.md").write_text(name + "\n")
        sources[name] = src

    out = base / "out"
    env = os.environ.copy()
    env.update(
        {
            "OUT": str(out),
            "PS5_VULKAN_SOURCE_DIR": str(sources["vulkan"]),
            "PS5_MESA_SOURCE_DIR": str(sources["mesa"]),
            "PS5_SDK_SOURCE_DIR": str(sources["sdk"]),
        }
    )
    subprocess.run(["bash", str(PREP)], cwd=ROOT, env=env, check=True)

    expected = {
        "PS5_Vulkan": PINS["vulkan"],
        "PS5_Mesa": PINS["mesa"],
        "PS5_PayloadSDK": PINS["sdk"],
    }
    for name, pin in expected.items():
        staged = out / name
        assert (staged / ".native-emus-pin").read_text().strip() == pin
        assert (staged / ".native-emus-source-mode").read_text().strip() == "verified-local-override"

    bad = base / "bad"
    shutil.copytree(sources["vulkan"], bad)
    (bad / ".native-emus-pin").write_text("0" * 40 + "\n")
    env["PS5_VULKAN_SOURCE_DIR"] = str(bad)
    failed = subprocess.run(["bash", str(PREP)], cwd=ROOT, env=env)
    assert failed.returncode != 0, "mismatched override pin must be rejected"

print("RADV verified-source overrides: PASS")
