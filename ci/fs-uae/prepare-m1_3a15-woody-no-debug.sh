#!/bin/sh
set -eu

AROS_URL=${AROS_URL:-https://sourceforge.net/projects/aros/files/nightly2/20260908/Binaries/AROS-20260908-amiga-m68k-boot-iso.zip/download}
AROS_SHA256=${AROS_SHA256:-799a327bdd50c008c7d35fe57524876b0780d4db4caf8df1cea6fcaf123cd46b}
AROS_ISO_SHA256=${AROS_ISO_SHA256:-a9087e8095c58d554a4b3bc85dd35ac9d2da6954bcc066a419d8a7a06c358e65}
BUNDLE=${BUNDLE:-out/m68kdeb-amiga}
OUT=${OUT:-out/m1-3a15}

for c in curl unzip xorriso sha256sum wc; do command -v "$c" >/dev/null; done
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
for f in S/Startup-Sequence C/Stack C/Echo C/Wait; do [ -e "$OUT/aros-root/$f" ] || exit 1; done
for f in amiboot woody-linux.bin; do [ -f "$BUNDLE/$f" ] || { echo "missing $BUNDLE/$f" >&2; exit 1; }; done
cp "$OUT/aros-root/S/Startup-Sequence" "$OUT/ORIGINAL-Startup-Sequence"
cp "$BUNDLE/amiboot" "$OUT/aros-root/amiboot"
cp "$BUNDLE/woody-linux.bin" "$OUT/aros-root/woody-linux.bin"

cat > "$OUT/aros-root/S/Startup-Sequence" <<'EOF'
C:Echo "M1.3a.15 Startup-Sequence reached" >SYS:m1-3a15-startup.marker
C:Stack 100000
C:Echo "M1.3a.15 pre-handoff boundary reached; amiboot debug disabled" >SYS:m1-3a15-pre-handoff.marker
SYS:amiboot -k SYS:woody-linux.bin root=/dev/ram video=pal console=ttyS0,9600n8
C:Echo "M1.3a.15 amiboot returned to AROS" >SYS:m1-3a15-amiboot-returned.marker
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
 echo "zorro_iii_memory_kib=65536"
 echo "amiboot_debug=disabled"
 echo "hard_drive_0=$OUT/aros-root"
 echo "hard_drive_1=absent"
 echo "payload=amiboot + Debian 3.0 woody linux.bin"
 echo "hypothesis=prior stalls were amiboot -d interactive debug wait"
 sha256sum "$OUT/rom/aros-rom.bin" "$OUT/rom/aros-ext.bin" "$OUT/aros-root/amiboot" "$OUT/aros-root/woody-linux.bin" "$OUT/aros-root/S/Startup-Sequence"
} > "$OUT/RUNTIME_PROVENANCE.txt"

echo 'PASS: prepared M1.3a.15 no-debug handoff control'
