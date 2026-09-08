#!/bin/sh
set -eu

OUT=${OUT:-out/aros-m68k}
INDEX_URL=${AROS_INDEX_URL:-https://www.aros.org/cgi-bin/files?lang=en&type=nightly2}
mkdir -p "$OUT"

INDEX="$OUT/nightly.html"
curl -fL --retry 3 --retry-delay 2 "$INDEX_URL" -o "$INDEX"

# Resolve the official amiga-m68k boot ISO link from the AROS nightly index.
# Keep discovery dynamic here; qualification records the resolved URL and hash.
URL=$(python3 - "$INDEX" "$INDEX_URL" <<'PY'
import re, sys
from html import unescape
from urllib.parse import urljoin

path, base = sys.argv[1:]
text = open(path, encoding='utf-8', errors='replace').read()
links = re.findall(r'href=["\']([^"\']+)["\']', text, re.I)
links = [unescape(x) for x in links]
for href in links:
    low = href.lower()
    if 'amiga-m68k-boot-iso' in low:
        print(urljoin(base, href))
        break
else:
    raise SystemExit('could not resolve amiga-m68k-boot-iso link from AROS nightly index')
PY
)

printf '%s\n' "$URL" > "$OUT/SOURCE_URL"

ARCHIVE="$OUT/aros-amiga-m68k-boot-iso"
curl -fL --retry 3 --retry-delay 2 "$URL" -o "$ARCHIVE"
sha256sum "$ARCHIVE" > "$OUT/SHA256SUMS"
file "$ARCHIVE" | tee "$OUT/FILE.txt"

# Determine archive/container format and extract enough metadata for the next
# runtime stage. AROS has historically shipped this target as compressed
# archives containing an ISO image.
mkdir -p "$OUT/extracted"
case "$(file -b "$ARCHIVE")" in
  *"LHA archive"*|*"LHa"*)
    command -v lha >/dev/null
    lha xw="$OUT/extracted" "$ARCHIVE" >/dev/null
    ;;
  *"Zip archive"*)
    command -v unzip >/dev/null
    unzip -q "$ARCHIVE" -d "$OUT/extracted"
    ;;
  *"XZ compressed"*)
    cp "$ARCHIVE" "$OUT/extracted/payload.xz"
    xz -dk "$OUT/extracted/payload.xz"
    ;;
  *"ISO 9660"*)
    cp "$ARCHIVE" "$OUT/extracted/aros-amiga-m68k.iso"
    ;;
  *)
    # Try common formats by signature/tool before failing.
    if unzip -t "$ARCHIVE" >/dev/null 2>&1; then
      unzip -q "$ARCHIVE" -d "$OUT/extracted"
    elif lha l "$ARCHIVE" >/dev/null 2>&1; then
      lha xw="$OUT/extracted" "$ARCHIVE" >/dev/null
    else
      echo "unsupported AROS nightly payload format" >&2
      exit 1
    fi
    ;;
esac

find "$OUT/extracted" -maxdepth 3 -type f -printf '%P\n' | sort > "$OUT/CONTENTS.txt"
ISO=$(find "$OUT/extracted" -type f \( -iname '*.iso' -o -iname '*boot*iso*' \) | head -n 1 || true)
if [ -z "$ISO" ]; then
  echo "no ISO image found in AROS amiga-m68k nightly payload" >&2
  cat "$OUT/CONTENTS.txt" >&2
  exit 1
fi
printf '%s\n' "$ISO" > "$OUT/ISO_PATH"
sha256sum "$ISO" > "$OUT/ISO_SHA256"

printf 'AROS source: %s\n' "$URL"
printf 'AROS ISO: %s\n' "$ISO"
cat "$OUT/ISO_SHA256"
