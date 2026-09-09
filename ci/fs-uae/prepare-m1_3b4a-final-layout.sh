#!/bin/sh
set -eu

LOADER=${LOADER:-out/m68kdeb-loader/m68kdeb-loader}
LAYOUT_PROBE=${LAYOUT_PROBE:-out/m68kdeb-loader/m68kdeb-layout-probe}
KERNEL=${KERNEL:-out/kernel-amiga/vmlinux-m68k-amiga}
INITRAMFS=${INITRAMFS:-out/initramfs-amiga/initramfs-m68kdeb.gz}
OUT=${OUT:-out/m1-3b4a}

for c in curl unzip xorriso sha256sum; do command -v "$c" >/dev/null; done
[ -f "$LOADER" ] || { echo "missing loader: $LOADER" >&2; exit 1; }
[ -f "$LAYOUT_PROBE" ] || { echo "missing layout probe: $LAYOUT_PROBE" >&2; exit 1; }
[ -f "$KERNEL" ] || { echo "missing kernel: $KERNEL" >&2; exit 1; }
[ -f "$INITRAMFS" ] || { echo "missing initramfs: $INITRAMFS" >&2; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/download" "$OUT/aros-root" "$OUT/rom" "$OUT/logs" "$OUT/screenshots"

if [ -n "${AROS_URL:-}" ] || [ -n "${AROS_SHA256:-}" ] || [ -n "${AROS_ISO_SHA256:-}" ]; then
  [ -n "${AROS_URL:-}" ] && [ -n "${AROS_SHA256:-}" ] && [ -n "${AROS_ISO_SHA256:-}" ] || {
    echo 'AROS_URL, AROS_SHA256 and AROS_ISO_SHA256 must be supplied together' >&2
    exit 1
  }
  ZIP="$OUT/download/aros-amiga-m68k-boot-iso.zip"
  curl -fL --retry 3 --retry-delay 2 "$AROS_URL" -o "$ZIP"
  printf '%s  %s\n' "$AROS_SHA256" "$ZIP" | sha256sum -c -
  unzip -q "$ZIP" -d "$OUT/download/extracted"
  ISO=$(find "$OUT/download/extracted" -type f -name '*.iso' | head -n 1 || true)
  [ -n "$ISO" ] || { echo 'AROS ISO not found' >&2; exit 1; }
  printf '%s  %s\n' "$AROS_ISO_SHA256" "$ISO" | sha256sum -c -
  AROS_RESOLUTION=pinned
else
  FETCH_OUT="$OUT/aros-fetch"
  OUT="$FETCH_OUT" sh ci/fs-uae/fetch-aros-m68k-nightly.sh
  AROS_URL=$(cat "$FETCH_OUT/SOURCE_URL")
  AROS_SHA256=$(awk 'NR == 1 { print $1 }' "$FETCH_OUT/SHA256SUMS")
  ISO=$(cat "$FETCH_OUT/ISO_PATH")
  AROS_ISO_SHA256=$(awk 'NR == 1 { print $1 }' "$FETCH_OUT/ISO_SHA256")
  [ -f "$ISO" ] || { echo "resolved AROS ISO missing: $ISO" >&2; exit 1; }
  AROS_RESOLUTION=nightly-index
fi

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
cp "$LOADER" "$OUT/aros-root/m68kdeb-loader"
cp "$LAYOUT_PROBE" "$OUT/aros-root/m68kdeb-layout-probe"
cp "$KERNEL" "$OUT/aros-root/vmlinux-m68k-amiga"
cp "$INITRAMFS" "$OUT/aros-root/initramfs-m68kdeb.gz"
rm -f "$OUT/aros-root/m1-3b4a-"*.marker \
      "$OUT/aros-root/m68kdeb-bootinfo-report.txt" \
      "$OUT/aros-root/m68kdeb-layout-report.txt"

cat > "$OUT/aros-root/S/Startup-Sequence" <<'EOF'
C:Echo "M1.3b.4a Startup-Sequence reached" >SYS:m1-3b4a-startup.marker
SYS:m68kdeb-loader SYS:vmlinux-m68k-amiga SYS:initramfs-m68kdeb.gz 5 >SYS:m68kdeb-bootinfo-report.txt
C:Echo "M1.3b.4a bootinfo preflight returned" >SYS:m1-3b4a-preflight-returned.marker
SYS:m68kdeb-layout-probe SYS:vmlinux-m68k-amiga SYS:initramfs-m68kdeb.gz 5 >SYS:m68kdeb-layout-report.txt
C:Echo "M1.3b.4a layout probe returned" >SYS:m1-3b4a-layout-returned.marker
C:Wait 300
EOF

{
  echo "aros_resolution=$AROS_RESOLUTION"
  echo "aros_source=$AROS_URL"
  echo "aros_payload_sha256=$AROS_SHA256"
  echo "aros_iso_sha256=$AROS_ISO_SHA256"
  echo "machine=A1200"
  echo "amiga_model_id=5"
  echo "cpu=68030"
  echo "mmu=68030"
  echo "cpu_speed=real"
  echo "zorro_iii_memory_kib=65536"
  echo "loader_role=M1.3b.3 bootinfo preflight followed by reversible final PT_LOAD layout probe"
  echo "linux_version=7.2"
  echo "linux_handoff=not_attempted"
  sha256sum "$OUT/rom/aros-rom.bin" "$OUT/rom/aros-ext.bin" \
    "$OUT/aros-root/m68kdeb-loader" "$OUT/aros-root/m68kdeb-layout-probe" \
    "$OUT/aros-root/vmlinux-m68k-amiga" "$OUT/aros-root/initramfs-m68kdeb.gz" \
    "$OUT/aros-root/S/Startup-Sequence"
} > "$OUT/RUNTIME_PROVENANCE.txt"

echo 'PASS: prepared M1.3b.4a final-layout runtime'
