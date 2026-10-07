#!/usr/bin/env python3
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import subprocess

def sha256(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--output-dir", required=True, type=pathlib.Path)
    ap.add_argument("--system", required=True)
    ap.add_argument("--core", required=True)
    ap.add_argument("--upstream", required=True)
    ap.add_argument("--pin", required=True)
    ap.add_argument("--cpu-backend", required=True)
    ap.add_argument("--graphics-backend", required=True)
    ap.add_argument("--artifact", action="append", default=[])
    args = ap.parse_args()

    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)

    artifacts = []
    for item in args.artifact:
        p = pathlib.Path(item)
        if not p.is_absolute():
            p = out / p
        if not p.is_file():
            raise SystemExit(f"artifact missing: {p}")
        artifacts.append({
            "name": p.name,
            "size": p.stat().st_size,
            "sha256": sha256(p),
        })

    try:
        project_commit = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], text=True
        ).strip()
    except Exception:
        project_commit = "unknown"

    manifest = {
        "schema_version": 1,
        "project": "NATIVE-EMUS-PS5",
        "project_commit": project_commit,
        "system": args.system,
        "core": args.core,
        "upstream": args.upstream,
        "upstream_pin": args.pin,
        "cpu_backend": args.cpu_backend,
        "graphics_backend": args.graphics_backend,
        "artifacts": artifacts,
    }

    (out / "BUILD-MANIFEST.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
