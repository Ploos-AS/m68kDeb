#!/bin/sh
set -eu

HARNESS=${HARNESS:-out/m1-3b4b5d/m68kdeb-takeover-5d}
OUT=${OUT:-out/m1-3b4b5d}

for c in curl unzip xorriso sha256sum; do command -v "$c" >/dev/null; done
[ -f "$HARNESS" ] || { echo "missing harness: $HARNESS" >&2; exit 1; }

mkdir -p "$OUT"
rm -rf "$OUT/aros-root" "$OUT/rom" "$OUT/logs" "$OUT/screenshots" "$OUT/aros-fetch"
mkdir -p "$OUT/aros-root" "$OUT/rom" "$OUT/logs" "$OUT/screenshots"

FETCH_OUT="$OUT/aros-fetch"
OUT="$FETCH_OUT" sh ci/fs-uae/fetch-aros-m68k-nightly.sh
AROS_URL=$(cat "$FETCH_OUT/SOURCE_URL")
AROS_SHA256=$(awk 'NR == 1 { print $1 }' "$FETCH_OUT/SHA256SUMS")
ISO=$(cat "$FETCH_OUT/ISO_PATH")
AROS_ISO_SHA256=$(awk 'NR == 1 { print $1 }' "$FETCH_OUT/ISO_SHA256")
[ -f "$ISO" ] || { echo "resolved AROS ISO missing: $ISO" >&2; exit 1; }

xorriso -osirrox on -indev "$ISO" -extract / "$OUT/aros-root" >/dev/null 2>&1
chmod -R u+rwX "$OUT/aros-root"
ROM=$(find "$OUT/aros-root" -type f -name 'aros-rom.bin' | head -n 1 || true)
EXT=$(find "$OUT/aros-root" -type f -name 'aros-ext.bin' | head -n 1 || true)
[ -n "$ROM" ] && [ -n "$EXT" ] || { echo 'matching AROS ROM pair missing' >&2; exit 1; }
cp "$ROM" "$OUT/rom/aros-rom.bin"
cp "$EXT" "$OUT/rom/aros-ext.bin"

for f in S/Startup-Sequence C/Echo C/Wait; do
  [ -e "$OUT/aros-root/$f" ] || { echo "missing AROS runtime file: $f" >&2; exit 1; }
done

cp "$OUT/aros-root/S/Startup-Sequence" "$OUT/ORIGINAL-Startup-Sequence"
cp "$HARNESS" "$OUT/aros-root/m68kdeb-takeover-5d"
rm -f "$OUT/aros-root/m1-3b4b5d-"*.marker "$OUT/aros-root/m68kdeb-5d-report.txt"

cat > "$OUT/aros-root/S/Startup-Sequence" <<'EOF'
C:Echo "M1.3b.4b.5d Startup-Sequence reached" >SYS:m1-3b4b5d-startup.marker
SYS:m68kdeb-takeover-5d >SYS:m68kdeb-5d-report.txt
C:Echo "ERROR: irreversible harness returned" >SYS:m1-3b4b5d-returned.marker
C:Wait 300
EOF

{
  echo "aros_source=$AROS_URL"
  echo "aros_payload_sha256=$AROS_SHA256"
  echo "aros_iso_sha256=$AROS_ISO_SHA256"
  echo "machine=A1200"
  echo "amiga_model_id=5"
  echo "cpu=68030"
  echo "mmu=68030"
  echo "zorro_iii_memory_kib=65536"
  echo "role=M1.3b.4b.5d guarded irreversible takeover runtime"
  echo "linux_entry_transfer=not_implemented"
  echo "expected_return=no"
  sha256sum "$OUT/rom/aros-rom.bin" "$OUT/rom/aros-ext.bin" \
    "$OUT/aros-root/m68kdeb-takeover-5d" "$OUT/aros-root/S/Startup-Sequence"
} > "$OUT/RUNTIME_PROVENANCE.txt"

echo 'PASS: prepared M1.3b.4b.5d irreversible runtime'
