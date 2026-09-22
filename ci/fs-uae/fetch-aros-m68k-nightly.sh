#!/bin/sh
set -eu

OUT=${OUT:-out/aros-m68k}
# Use the TLS-valid SourceForge-hosted AROS site. The www.aros.org hostname
# currently presents a certificate that does not match the host name, so CI
# deliberately avoids bypassing TLS verification.
INDEX_URL=${AROS_INDEX_URL:-https://aros.sourceforge.io/cgi-bin/files?lang=en&type=nightly2}
PINNED_URL=${AROS_PINNED_URL:-}
mkdir -p "$OUT"

INDEX="$OUT/nightly.html"
if [ -n "$PINNED_URL" ]; then
  URL="$PINNED_URL"
else
  curl -fL --retry 3 --retry-delay 2 "$INDEX_URL" -o "$INDEX"

# Resolve the official amiga-m68k boot ISO link from the AROS nightly index.
# The index labels the target in table text while the href itself is only a
# generic SourceForge Download URL, so do not require the target name to occur
# inside href. Prefer the first Download link after the amiga-m68k-boot-iso
# label; retain the old href-name match as a compatibility fallback.
URL=$(python3 - "$INDEX" "$INDEX_URL" <<'PY'
import re, sys
from html import unescape
from urllib.parse import urljoin

path, base = sys.argv[1:]
text = open(path, encoding='utf-8', errors='replace').read()

# Current AROS index: target label is outside the anchor. Limit the search to
# the following table fragment so we select this target's Download link rather
# than another nightly artifact.
m = re.search(r'amiga-m68k-boot-iso(?P<tail>.{0,12000})', text, re.I | re.S)
if m:
    hrefs = re.findall(r'href=["\']([^"\']+)["\']', m.group('tail'), re.I)
    for href in hrefs:
        href = unescape(href)
        if 'sourceforge.net' in href.lower() or '/projects/aros/' in href.lower():
            print(urljoin(base, href))
            raise SystemExit(0)

# Compatibility with older index layouts that included the artifact name in
# the URL itself.
links = [unescape(x) for x in re.findall(r'href=["\']([^"\']+)["\']', text, re.I)]
for href in links:
    if 'amiga-m68k-boot-iso' in href.lower():
        print(urljoin(base, href))
        raise SystemExit(0)

raise SystemExit('could not resolve amiga-m68k-boot-iso link from AROS nightly index')
PY
)
fi

printf '%s\n' "$URL" > "$OUT/SOURCE_URL"

ARCHIVE="$OUT/aros-amiga-m68k-boot-iso"
# The nightly index can briefly advertise a dated SourceForge artifact before
# that artifact is actually published. Fall back to the last known-good AROS
# amiga-m68k boot ISO instead of turning emulator qualification into a 404.
FALLBACK_URL=${AROS_FALLBACK_URL:-https://sourceforge.net/projects/aros/files/nightly2/20260919/Binaries/AROS-20260919-amiga-m68k-boot-iso.zip/download}
if ! curl -fL --retry 3 --retry-delay 2 "$URL" -o "$ARCHIVE"; then
  if [ "$URL" = "$FALLBACK_URL" ]; then
    exit 1
  fi
  echo "AROS nightly unavailable; falling back to known-good payload" >&2
  URL="$FALLBACK_URL"
  printf '%s\n' "$URL" > "$OUT/SOURCE_URL"
  curl -fL --retry 3 --retry-delay 2 "$URL" -o "$ARCHIVE"
fi
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
