#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
MANIFEST="$ROOT/upstreams/scene-cache.json"
OUT="$ROOT/build/ps5-scene-reference-cache"
ONLY=""

usage() {
  cat <<'EOF'
Usage: cache_scene_references.sh [--manifest FILE] [--output DIR] [--only repo/name]

Materializes source snapshots for entries with cache_source=true in
upstreams/scene-cache.json. Each snapshot is pinned to the exact recorded
commit, includes initialized submodules where available, and is hashed.

This is a preservation/cache step only. It never imports reference code into
NATIVE-EMUS-PS5.
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --manifest)
      MANIFEST="$2"; shift 2 ;;
    --output)
      OUT="$2"; shift 2 ;;
    --only)
      ONLY="$2"; shift 2 ;;
    -h|--help)
      usage; exit 0 ;;
    *)
      echo "unknown argument: $1" >&2
      usage >&2
      exit 2 ;;
  esac
done

for tool in git python3 tar sha256sum; do
  command -v "$tool" >/dev/null || {
    echo "missing required tool: $tool" >&2
    exit 2
  }
done

[[ -f "$MANIFEST" ]] || {
  echo "manifest not found: $MANIFEST" >&2
  exit 2
}

rm -rf "$OUT"
mkdir -p "$OUT"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

INDEX="$OUT/CACHE-INDEX.tsv"
printf 'repository\tpin\tlicense\tarchive\tsha256\n' > "$INDEX"

python3 - "$MANIFEST" "$ONLY" <<'PY' > "$TMP/entries.tsv"
import json
import sys

manifest, only = sys.argv[1], sys.argv[2]
with open(manifest, encoding="utf-8") as f:
    data = json.load(f)

for item in data.get("references", []):
    if not item.get("cache_source", False):
        continue
    repo = item["repository"]
    if only and repo != only:
        continue
    fields = [
        repo,
        item["url"],
        item["pin"],
        item.get("license", "unknown"),
    ]
    if any("\t" in value or "\n" in value for value in fields):
        raise SystemExit(f"invalid tab/newline in cache manifest entry: {repo}")
    print("\t".join(fields))
PY

if [[ ! -s "$TMP/entries.tsv" ]]; then
  echo "no cacheable references selected" >&2
  exit 3
fi

while IFS=$'\t' read -r repository url pin license; do
  safe="${repository//\//__}"
  work="$TMP/$safe"
  archive="$OUT/${safe}-${pin}.tar.gz"

  echo "==> cache $repository @ $pin"
  GIT_TERMINAL_PROMPT=0 git clone \
    --filter=blob:none \
    --no-checkout \
    "$url" "$work"

  GIT_TERMINAL_PROMPT=0 git -C "$work" fetch --depth=1 origin "$pin"
  git -C "$work" checkout --detach "$pin"
  test "$(git -C "$work" rev-parse HEAD)" = "$pin"

  if [[ -f "$work/.gitmodules" ]]; then
    GIT_TERMINAL_PROMPT=0 git -C "$work" submodule update \
      --init --recursive --depth 1
  fi

  tar \
    --exclude='.git' \
    --exclude='*/.git' \
    -C "$TMP" \
    -czf "$archive" \
    "$safe"

  tar -tzf "$archive" >/dev/null
  digest="$(sha256sum "$archive" | awk '{print $1}')"
  printf '%s\t%s\t%s\t%s\t%s\n' \
    "$repository" "$pin" "$license" "$(basename "$archive")" "$digest" \
    >> "$INDEX"

  rm -rf "$work"
done < "$TMP/entries.tsv"

(
  cd "$OUT"
  sha256sum ./*.tar.gz > SHA256SUMS
)

python3 - "$MANIFEST" "$INDEX" "$OUT/CACHE-METADATA.json" <<'PY'
import csv
import json
import sys
from pathlib import Path

manifest_path, index_path, output_path = map(Path, sys.argv[1:4])
manifest = json.loads(manifest_path.read_text(encoding="utf-8"))

rows = []
with index_path.open(encoding="utf-8", newline="") as f:
    rows.extend(csv.DictReader(f, delimiter="\t"))

out = {
    "schema_version": 1,
    "source_manifest": str(manifest_path),
    "source_manifest_updated": manifest.get("updated"),
    "snapshots": rows,
}
output_path.write_text(json.dumps(out, indent=2) + "\n", encoding="utf-8")
PY

echo "Reference cache complete: $OUT"
echo "Index: $INDEX"
echo "Hashes: $OUT/SHA256SUMS"
