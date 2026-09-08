#!/bin/sh
set -eu

DIR=${1:-out/m68kdeb-amiga}

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

for f in amiboot vmlinux-m68k-amiga initramfs-m68kdeb.gz Start-m68kDeb Start-m68kDeb-Serial SHA256SUMS PROVENANCE.txt; do
  [ -f "$DIR/$f" ] || fail "missing $DIR/$f"
done

[ -s "$DIR/amiboot" ] || fail "amiboot is empty"
[ -s "$DIR/vmlinux-m68k-amiga" ] || fail "kernel is empty"
[ -s "$DIR/initramfs-m68kdeb.gz" ] || fail "initramfs is empty"
[ -x "$DIR/Start-m68kDeb" ] || fail "Start-m68kDeb is not executable"
[ -x "$DIR/Start-m68kDeb-Serial" ] || fail "Start-m68kDeb-Serial is not executable"
grep -q 'vmlinux-m68k-amiga' "$DIR/Start-m68kDeb" || fail "normal launcher lacks uncompressed kernel"
grep -q 'initramfs-m68kdeb.gz' "$DIR/Start-m68kDeb" || fail "normal launcher lacks initramfs"
grep -q 'console=ttyS0,9600n8' "$DIR/Start-m68kDeb-Serial" || fail "serial launcher lacks serial console"
grep -q 'amiboot_source=' "$DIR/PROVENANCE.txt" || fail "provenance lacks amiboot source"
[ -s "$DIR/SHA256SUMS" ] || fail "SHA256SUMS is empty"
[ -s "$DIR/PROVENANCE.txt" ] || fail "PROVENANCE.txt is empty"
(cd "$DIR" && sha256sum -c SHA256SUMS)

echo "PASS: Amiga bootstrap bundle shape"
