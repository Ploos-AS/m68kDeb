#!/bin/sh
set -eu

# Resolve AROS through the shared nightly fetcher; it records the exact
# resolved source and checksums while tolerating vanished dated mirrors.
BUNDLE=${BUNDLE:-out/m68kdeb-amiga}
OUT=${OUT:-out/m1-3a11}

command -v curl >/dev/null
command -v unzip >/dev/null
command -v xorriso >/dev/null
command -v sha256sum >/dev/null

rm -rf "$OUT"
mkdir -p "$OUT/download" "$OUT/aros-root" "$OUT/rom" "$OUT/logs" "$OUT/screenshots"

FETCH_OUT="$OUT/aros-fetch"
OUT="$FETCH_OUT" sh ci/fs-uae/fetch-aros-m68k-nightly.sh
AROS_URL=$(cat "$FETCH_OUT/SOURCE_URL")
AROS_SHA256=$(awk 'NR == 1 { print $1 }' "$FETCH_OUT/SHA256SUMS")
ISO=$(cat "$FETCH_OUT/ISO_PATH")
AROS_ISO_SHA256=$(awk 'NR == 1 { print $1 }' "$FETCH_OUT/ISO_SHA256")

xorriso -osirrox on -indev "$ISO" -extract / "$OUT/aros-root" >/dev/null 2>&1
chmod -R u+rwX "$OUT/aros-root"

ROM=$(find "$OUT/aros-root" -type f -name 'aros-rom.bin' | head -n 1 || true)
EXT=$(find "$OUT/aros-root" -type f -name 'aros-ext.bin' | head -n 1 || true)
[ -n "$ROM" ] || { echo 'aros-rom.bin not found in pinned AROS ISO' >&2; exit 1; }
[ -n "$EXT" ] || { echo 'aros-ext.bin not found in pinned AROS ISO' >&2; exit 1; }
cp "$ROM" "$OUT/rom/aros-rom.bin"
cp "$EXT" "$OUT/rom/aros-ext.bin"

for f in S/Startup-Sequence C/Stack C/Echo C/Wait; do
  [ -e "$OUT/aros-root/$f" ] || { echo "missing AROS runtime file: $f" >&2; exit 1; }
done
for f in amiboot vmlinux-m68k-amiga initramfs-m68kdeb.gz; do
  [ -f "$BUNDLE/$f" ] || { echo "missing probe payload: $BUNDLE/$f" >&2; exit 1; }
done

cp "$OUT/aros-root/S/Startup-Sequence" "$OUT/ORIGINAL-Startup-Sequence"
cp "$BUNDLE/amiboot" "$OUT/aros-root/amiboot"
cp "$BUNDLE/vmlinux-m68k-amiga" "$OUT/aros-root/vmlinux-m68k-amiga"\ncp "$BUNDLE/initramfs-m68kdeb.gz" "$OUT/aros-root/initramfs-m68kdeb.gz"
rm -f "$OUT/aros-root/m1-3a11-startup.marker" "$OUT/aros-root/m1-3a11-invoking-kernel.marker"

cat > "$OUT/aros-root/S/Startup-Sequence" <<'EOF'
C:Echo "M1.3a.11 Startup-Sequence reached" >SYS:m1-3a11-startup.marker
C:Stack 100000
C:Echo "M1.3a.11 invoking amiboot kernel-only probe" >SYS:m1-3a11-invoking-kernel.marker
SYS:amiboot -k SYS:vmlinux-m68k-amiga -r SYS:initramfs-m68kdeb.gz root=/dev/ram rdinit=/init video=pal console=ttyS0,9600n8
C:Echo "M1.3a.11 amiboot returned unexpectedly"
C:Wait 300
EOF

{
  echo "aros_source=$AROS_URL"
  echo "aros_payload_sha256=$AROS_SHA256"
  echo "aros_iso_sha256=$AROS_ISO_SHA256"
  echo "machine=A1200"
  echo "cpu=68030"
  echo "mmu=68030"
  echo "cpu_speed=real"
  echo "accelerator=none"
  echo "accelerator_rom=none"
  echo "rom_source=pinned AROS 2026-09-08 nightly ISO"
  echo "hard_drive_0=$OUT/aros-root"
  echo "hard_drive_1=absent"
  echo "payload=amiboot + vmlinux-m68k-amiga + initramfs-m68kdeb.gz on DH0"
  echo "initramfs=initramfs-m68kdeb.gz"
  echo "probe=observe Linux initramfs userspace marker after amiboot invocation"
  sha256sum "$OUT/rom/aros-rom.bin" "$OUT/rom/aros-ext.bin" \
    "$OUT/aros-root/amiboot" "$OUT/aros-root/vmlinux-m68k-amiga" "$OUT/aros-root/initramfs-m68kdeb.gz" \
    "$OUT/aros-root/S/Startup-Sequence" "$OUT/ORIGINAL-Startup-Sequence"
} > "$OUT/RUNTIME_PROVENANCE.txt"

echo "PASS: prepared M1.3a.11 single-DH0 AROS root with amiboot + kernel only"
