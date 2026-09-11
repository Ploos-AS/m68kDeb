#!/bin/sh
set -eu

LOADER=${LOADER:-out/m1-3b4b6b/m68kdeb-loader-68030-linux-transfer-qual}
KERNEL=${KERNEL:-out/kernel-amiga/vmlinux-m68k-amiga}
INITRAMFS=${INITRAMFS:-out/initramfs-amiga/initramfs-m68kdeb.gz}
OUT=${OUT:-out/m1-3b4b6b}

for c in curl unzip xorriso sha256sum; do command -v "$c" >/dev/null; done
[ -f "$LOADER" ] || { echo "missing loader: $LOADER" >&2; exit 1; }
[ -f "$KERNEL" ] || { echo "missing kernel: $KERNEL" >&2; exit 1; }
[ -f "$INITRAMFS" ] || { echo "missing initramfs: $INITRAMFS" >&2; exit 1; }

rm -rf "$OUT/aros-root" "$OUT/rom" "$OUT/logs" "$OUT/screenshots" "$OUT/aros-fetch"
mkdir -p "$OUT/aros-root" "$OUT/rom" "$OUT/logs" "$OUT/screenshots"

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
[ -n "$ROM" ] && [ -n "$EXT" ] || { echo 'matching AROS ROM pair missing' >&2; exit 1; }
cp "$ROM" "$OUT/rom/aros-rom.bin"
cp "$EXT" "$OUT/rom/aros-ext.bin"

for f in S/Startup-Sequence C/Echo C/Wait; do
  [ -e "$OUT/aros-root/$f" ] || { echo "missing AROS runtime file: $f" >&2; exit 1; }
done

cp "$OUT/aros-root/S/Startup-Sequence" "$OUT/ORIGINAL-Startup-Sequence"
cp "$LOADER" "$OUT/aros-root/m68kdeb-loader-68030-linux-transfer-qual"
cp "$KERNEL" "$OUT/aros-root/vmlinux-m68k-amiga"
cp "$INITRAMFS" "$OUT/aros-root/initramfs-m68kdeb.gz"
rm -f "$OUT/aros-root/m1-3b4b6b-"*.marker "$OUT/aros-root/m68kdeb-6b-report.txt"

cat > "$OUT/aros-root/S/Startup-Sequence" <<'EOF'
C:Echo "M1.3b.4b.6b Startup-Sequence reached" >SYS:m1-3b4b6b-startup.marker
SYS:m68kdeb-loader-68030-linux-transfer-qual SYS:vmlinux-m68k-amiga SYS:initramfs-m68kdeb.gz 5 >SYS:m68kdeb-6b-report.txt
C:Echo "M1.3b.4b.6b loader returned unexpectedly" >SYS:m1-3b4b6b-returned.marker
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
  echo "role=M1.3b.4b.6b CI-only Linux transfer runtime qualification"
  echo "transition_entry_transfer=attempted_after_runtime_gate"
  echo "cache_mutation=attempted_in_emulator"
  echo "mmu_mutation=attempted_in_emulator"
  echo "pflusha=attempted_in_emulator"
  echo "linux_handoff=attempted"
  echo "success_evidence=initramfs serial marker"
  sha256sum "$OUT/rom/aros-rom.bin" "$OUT/rom/aros-ext.bin" \
    "$OUT/aros-root/m68kdeb-loader-68030-linux-transfer-qual" \
    "$OUT/aros-root/vmlinux-m68k-amiga" "$OUT/aros-root/initramfs-m68kdeb.gz" \
    "$OUT/aros-root/S/Startup-Sequence"
} > "$OUT/RUNTIME_PROVENANCE.txt"

echo 'PASS: prepared M1.3b.4b.6b Linux-transfer runtime'
