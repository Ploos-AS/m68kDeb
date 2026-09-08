#!/bin/sh
set -eu

AROS_URL=${AROS_URL:-https://sourceforge.net/projects/aros/files/nightly2/20260908/Binaries/AROS-20260908-amiga-m68k-boot-iso.zip/download}
AROS_SHA256=${AROS_SHA256:-799a327bdd50c008c7d35fe57524876b0780d4db4caf8df1cea6fcaf123cd46b}
AROS_ISO_SHA256=${AROS_ISO_SHA256:-a9087e8095c58d554a4b3bc85dd35ac9d2da6954bcc066a419d8a7a06c358e65}
BUNDLE=${BUNDLE:-out/m68kdeb-amiga}
OUT=${OUT:-out/m1-3a3}

command -v curl >/dev/null
command -v unzip >/dev/null
command -v xorriso >/dev/null
command -v sha256sum >/dev/null

rm -rf "$OUT"
mkdir -p "$OUT/download" "$OUT/aros-root" "$OUT/logs" "$OUT/screenshots"

ZIP="$OUT/download/aros-amiga-m68k-boot-iso.zip"
curl -fL --retry 3 --retry-delay 2 "$AROS_URL" -o "$ZIP"
printf '%s  %s\n' "$AROS_SHA256" "$ZIP" | sha256sum -c -

unzip -q "$ZIP" -d "$OUT/download/extracted"
ISO=$(find "$OUT/download/extracted" -type f -name '*.iso' | head -n 1 || true)
[ -n "$ISO" ] || { echo 'AROS ISO not found' >&2; exit 1; }
printf '%s  %s\n' "$AROS_ISO_SHA256" "$ISO" | sha256sum -c -

xorriso -osirrox on -indev "$ISO" -extract / "$OUT/aros-root" >/dev/null 2>&1
# ISO extraction preserves read-only media permissions. This tree is a disposable
# CI copy that must be writable so we can inject m68kDeb and replace Startup-Sequence.
chmod -R u+rwX "$OUT/aros-root"

for f in boot/amiga/aros-rom.bin boot/amiga/aros-ext.bin S/Startup-Sequence C/Stack C/Echo C/CD; do
  [ -e "$OUT/aros-root/$f" ] || { echo "missing AROS runtime file: $f" >&2; exit 1; }
done

for f in amiboot vmlinux-m68k-amiga initramfs-m68kdeb.gz; do
  [ -f "$BUNDLE/$f" ] || { echo "missing m68kDeb bundle file: $BUNDLE/$f" >&2; exit 1; }
done

mkdir -p "$OUT/aros-root/m68kdeb"
cp "$BUNDLE/amiboot" "$OUT/aros-root/m68kdeb/amiboot"
cp "$BUNDLE/vmlinux-m68k-amiga" "$OUT/aros-root/m68kdeb/vmlinux-m68k-amiga"
cp "$BUNDLE/initramfs-m68kdeb.gz" "$OUT/aros-root/m68kdeb/initramfs-m68kdeb.gz"
cp "$OUT/aros-root/S/Startup-Sequence" "$OUT/ORIGINAL-Startup-Sequence"

cat > "$OUT/aros-root/S/Startup-Sequence" <<'EOF'
C:Stack 100000
C:Echo "m68kDeb M1.3a.3 AROS bootstrap"
C:CD SYS:m68kdeb
SYS:m68kdeb/amiboot -d -k vmlinux-m68k-amiga -r initramfs-m68kdeb.gz root=/dev/ram video=pal console=ttyS0,9600n8
C:Echo "M1.3a.3: amiboot returned unexpectedly"
C:Wait 30
EOF

{
  echo "aros_source=$AROS_URL"
  echo "aros_payload_sha256=$AROS_SHA256"
  echo "aros_iso_sha256=$AROS_ISO_SHA256"
  echo "kickstart_file=$OUT/aros-root/boot/amiga/aros-rom.bin"
  echo "kickstart_ext_file=$OUT/aros-root/boot/amiga/aros-ext.bin"
  echo "amiboot_workdir=SYS:m68kdeb"
  echo "amiboot_kernel=vmlinux-m68k-amiga"
  echo "amiboot_initramfs=initramfs-m68kdeb.gz"
  sha256sum "$OUT/aros-root/m68kdeb/amiboot" \
    "$OUT/aros-root/m68kdeb/vmlinux-m68k-amiga" \
    "$OUT/aros-root/m68kdeb/initramfs-m68kdeb.gz"
} > "$OUT/RUNTIME_PROVENANCE.txt"

echo "PASS: prepared pinned AROS runtime root with m68kDeb bundle"
