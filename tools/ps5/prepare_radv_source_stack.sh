#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${OUT:-$ROOT/build/ps5/radv-stack}"

PS5_VULKAN_REPO="https://github.com/mihawk-99/PS5_Vulkan.git"
PS5_VULKAN_SLUG="mihawk-99/PS5_Vulkan"
PS5_VULKAN_PIN="71026e7ec1951fe72ae5b9118ff0905216a4c220"

PS5_MESA_REPO="https://github.com/mihawk-99/PS5_Mesa.git"
PS5_MESA_SLUG="mihawk-99/PS5_Mesa"
PS5_MESA_PIN="cedb774b27d089fa81f46add28d0a8c13ff0f7d2"

PS5_SDK_REPO="https://github.com/mihawk-99/PS5_PayloadSDK.git"
PS5_SDK_SLUG="mihawk-99/PS5_PayloadSDK"
PS5_SDK_PIN="95c08f27386fc698f6bbe21dde3030140a41d10b"

source_pin() {
  local dir=$1
  if [[ -d "$dir/.git" ]]; then
    git -C "$dir" rev-parse HEAD
    return
  fi
  if [[ -f "$dir/.native-emus-pin" ]]; then
    cat "$dir/.native-emus-pin"
    return
  fi
  return 1
}

stage_verified_source() {
  local source=$1
  local expected=$2
  local dir=$3
  local label=$4

  [[ -d "$source" ]] || {
    echo "$label override does not exist: $source" >&2
    exit 10
  }

  local actual
  actual="$(source_pin "$source" 2>/dev/null || true)"
  [[ "$actual" == "$expected" ]] || {
    echo "$label override pin mismatch: $actual (expected $expected)" >&2
    echo "Override directories must be a git checkout at the exact pin or carry .native-emus-pin." >&2
    exit 11
  }

  rm -rf "$dir"
  mkdir -p "$(dirname "$dir")"
  cp -a "$source" "$dir"
  printf '%s\n' "$expected" > "$dir/.native-emus-pin"
  printf 'verified-local-override\n' > "$dir/.native-emus-source-mode"
}

fetch_pin() {
  local url=$1
  local slug=$2
  local pin=$3
  local dir=$4

  rm -rf "$dir"
  mkdir -p "$(dirname "$dir")"

  if GIT_TERMINAL_PROMPT=0 git clone --filter=blob:none "$url" "$dir" 2>/tmp/native-emus-git-fetch.log; then
    git -C "$dir" checkout --detach "$pin"
    local actual
    actual="$(git -C "$dir" rev-parse HEAD)"
    [[ "$actual" == "$pin" ]] || {
      echo "pin mismatch for $url: $actual != $pin" >&2
      exit 2
    }
    printf '%s\n' "$pin" > "$dir/.native-emus-pin"
    printf 'git\n' > "$dir/.native-emus-source-mode"
    return
  fi

  echo "git clone unavailable for $slug; trying immutable codeload archive" >&2
  cat /tmp/native-emus-git-fetch.log >&2 || true

  local archive
  archive="$(mktemp)"
  local archive_url="https://codeload.github.com/$slug/tar.gz/$pin"
  if ! curl --fail --location --retry 3 --retry-all-errors "$archive_url" -o "$archive"; then
    rm -f "$archive"
    echo "Cannot fetch $slug@$pin from git or codeload." >&2
    echo "Provide the exact source through the corresponding *_SOURCE_DIR override." >&2
    exit 12
  fi
  [[ -s "$archive" ]] || {
    echo "empty codeload archive for $slug@$pin" >&2
    rm -f "$archive"
    exit 3
  }

  mkdir -p "$dir"
  tar -xzf "$archive" --strip-components=1 -C "$dir"
  rm -f "$archive"

  [[ -f "$dir/README.md" || -f "$dir/CMakeLists.txt" || -f "$dir/Makefile" ]] || {
    echo "unexpected source archive layout for $slug@$pin" >&2
    exit 4
  }
  printf '%s\n' "$pin" > "$dir/.native-emus-pin"
  printf 'codeload\n' > "$dir/.native-emus-source-mode"
}

prepare_one() {
  local override=$1
  local url=$2
  local slug=$3
  local pin=$4
  local dir=$5
  local label=$6

  if [[ -n "$override" ]]; then
    stage_verified_source "$override" "$pin" "$dir" "$label"
  else
    fetch_pin "$url" "$slug" "$pin" "$dir"
  fi
}

rm -rf "$OUT"
mkdir -p "$OUT"

prepare_one "${PS5_VULKAN_SOURCE_DIR:-}" "$PS5_VULKAN_REPO" "$PS5_VULKAN_SLUG" "$PS5_VULKAN_PIN" "$OUT/PS5_Vulkan" "PS5_Vulkan"
prepare_one "${PS5_MESA_SOURCE_DIR:-}" "$PS5_MESA_REPO" "$PS5_MESA_SLUG" "$PS5_MESA_PIN" "$OUT/PS5_Mesa" "PS5_Mesa"
prepare_one "${PS5_SDK_SOURCE_DIR:-}" "$PS5_SDK_REPO" "$PS5_SDK_SLUG" "$PS5_SDK_PIN" "$OUT/PS5_PayloadSDK" "PS5_PayloadSDK"

cat > "$OUT/PINS.txt" <<EOF
PS5_Vulkan=$PS5_VULKAN_PIN
PS5_Mesa=$PS5_MESA_PIN
PS5_PayloadSDK=$PS5_SDK_PIN
EOF

for dir in PS5_Vulkan PS5_Mesa PS5_PayloadSDK; do
  printf '%s source=%s pin=%s\n' \
    "$dir" \
    "$(cat "$OUT/$dir/.native-emus-source-mode")" \
    "$(cat "$OUT/$dir/.native-emus-pin")"
done

echo "Pinned RADV source stack prepared at: $OUT"
echo "Build RADV only from these exact revisions; never follow moving main branches."
