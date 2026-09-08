#!/bin/sh
set -eu

OUT=${OUT:-out/amiboot}
AMIBOOT_URL=${AMIBOOT_URL:-https://archive.debian.org/debian/dists/woody/main/disks-m68k/current/amiga/amiboot-5.6}
AMIBOOT_SHA256=${AMIBOOT_SHA256:-8aae977164de07dea64857a6aa6a47c2bf8cc02dc3bde0052c42975cb7e0df9d}

command -v curl >/dev/null
command -v sha256sum >/dev/null

rm -rf "$OUT"
mkdir -p "$OUT"

curl -fL "$AMIBOOT_URL" -o "$OUT/amiboot"
[ -s "$OUT/amiboot" ] || { echo 'FAIL: downloaded amiboot is empty' >&2; exit 1; }

ACTUAL_SHA256=$(sha256sum "$OUT/amiboot" | awk '{print $1}')
if [ "$ACTUAL_SHA256" != "$AMIBOOT_SHA256" ]; then
  echo "FAIL: amiboot SHA-256 mismatch" >&2
  echo "expected=$AMIBOOT_SHA256" >&2
  echo "actual=$ACTUAL_SHA256" >&2
  exit 1
fi

printf '%s\n' "$AMIBOOT_URL" > "$OUT/SOURCE_URL"
printf '%s  %s\n' "$ACTUAL_SHA256" amiboot > "$OUT/SHA256SUMS"
printf 'amiboot: %s\nsha256: %s\n' "$OUT/amiboot" "$ACTUAL_SHA256"
