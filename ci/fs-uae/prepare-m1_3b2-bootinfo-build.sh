#!/bin/sh
set -eu

AROS_URL=${AROS_URL:-https://sourceforge.net/projects/aros/files/nightly2/20260908/Binaries/AROS-20260908-amiga-m68k-boot-iso.zip/download}
AROS_SHA256=${AROS_SHA256:-799a327bdd50c008c7d35fe57524876b0780d4db4caf8df1cea6fcaf123cd46b}
AROS_ISO_SHA256=${AROS_ISO_SHA256:-a9087e8095c58d554a4b3bc85dd35ac9d2da6954bcc066a419d8a7a06c358e65}
LOADER=${LOADER:-out/m68kdeb-loader/m68kdeb-loader}
KERNEL=${KERNEL:-out/kernel-amiga/vmlinux-m68k-amiga}
OUT=${OUT:-out/m1-3b2}

for c in curl unzip xorriso sha256sum; do command -v "$c" >/dev/null; done
[ -f "$LOADER" ] || { echo "missing loader: $LOADER" >&2; exit 1; }
[ -f "$KERNEL" ] || { echo "missing kernel: $KERNEL" >&2; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/download" "$OUT/aros-root" "$OUT/rom" "$OUT/logs" "$OUT/screenshots"
ZIP="$OUT/download/aros-amiga-m68k-boot-iso.zip"
curl -fL --retry 3 --retry-delay 2 "$AROS_URL" -o "$ZIP"
printf '%s  %s\n' "$AROS_SHA256" "$ZIP" | sha256sum -c -
unzip -q "$ZIP" -d "$OUT/download/extracted"
ISO=$(find "$OUT/download/extracted" -type f -name '*.iso' | head -n 1 || true)
[ -n "$ISO" ] || { echo 'AROS ISO not found' >&2; exit 1; }
printf '%s  %s\n' "$AROS_ISO_SHA256" "$ISO" | sha256sum -c -
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
cp "$KERNEL" "$OUT/aros-root/vmlinux-m68k-amiga"
rm -f "$OUT/aros-root/m1-3b2-"*.marker "$OUT/aros-root/m68kdeb-bootinfo-report.txt"

cat > "$OUT/aros-root/S/Startup-Sequence" <<'EOF'
C:Echo "M1.3b.2 Startup-Sequence reached" >SYS:m1-3b2-startup.marker
SYS:m68kdeb-loader SYS:vmlinux-m68k-amiga 5 >SYS:m68kdeb-bootinfo-report.txt
C:Echo "M1.3b.2 loader returned" >SYS:m1-3b2-loader-returned.marker
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
  echo "cpu_speed=real"
  echo "zorro_iii_memory_kib=65536"
  echo "loader_role=kernel load + bootinfo construction/self-check; no handoff"
  echo "linux_version=7.2"
  echo "linux_handoff=not_attempted"
  sha256sum "$OUT/rom/aros-rom.bin" "$OUT/rom/aros-ext.bin" \
    "$OUT/aros-root/m68kdeb-loader" "$OUT/aros-root/vmlinux-m68k-amiga" \
    "$OUT/aros-root/S/Startup-Sequence"
} > "$OUT/RUNTIME_PROVENANCE.txt"

echo 'PASS: prepared M1.3b.2 kernel-load/bootinfo-build runtime'
