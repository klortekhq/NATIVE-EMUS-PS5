#!/usr/bin/env bash
set -euo pipefail

# Official ps5-payload-dev pacbrew release asset:
# https://github.com/ps5-payload-dev/pacbrew-repo/releases/tag/v0.40.2
# Asset id 526391261 publishes this SHA-256 through the GitHub Releases API.
readonly PACBREW_RELEASE="v0.40.2"
readonly PACBREW_SHA256="a85f65de418a8e6a898c6c3e3c870d50fff7618a200e4dd59ea9692af6ecec4d"
readonly PACBREW_ASSET="ps5-payload-dev.tar.gz"
readonly PACBREW_URL="https://github.com/ps5-payload-dev/pacbrew-repo/releases/download/${PACBREW_RELEASE}/${PACBREW_ASSET}"

tmp_root="${RUNNER_TEMP:-${TMPDIR:-/tmp}}"
archive="${tmp_root%/}/ps5-payload-dev-${PACBREW_RELEASE}.tar.gz"

rm -f "$archive"
wget -q "$PACBREW_URL" -O "$archive"
printf '%s  %s\n' "$PACBREW_SHA256" "$archive" |
  sha256sum --check --strict
sudo tar xf "$archive" -C /
test -x /opt/ps5-payload-sdk/bin/prospero-clang++
test -d /opt/ps5-payload-sdk/target/lib
