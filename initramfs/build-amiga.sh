#!/bin/sh
set -eu

OUT=${OUT:-out/initramfs-amiga}
ROOT="$OUT/root"
PORTS=${PORTS:-https://deb.debian.org/debian-ports}
SUITE=${SUITE:-unstable}

command -v curl >/dev/null
command -v gzip >/dev/null
command -v cpio >/dev/null
command -v dpkg-deb >/dev/null
command -v find >/dev/null

rm -rf "$OUT"
mkdir -p "$ROOT/bin" "$ROOT/sbin" "$ROOT/proc" "$ROOT/sys" "$ROOT/dev" "$ROOT/tmp" "$OUT/download"

# Resolve the current busybox-static m68k package directly from Debian Ports.
PACKAGES="$OUT/Packages.gz"
curl -fL "$PORTS/dists/$SUITE/main/binary-m68k/Packages.gz" -o "$PACKAGES"
PKG_PATH=$(gzip -dc "$PACKAGES" | awk '
  $0 == "Package: busybox-static" { hit=1; next }
  hit && /^Filename: / { sub(/^Filename: /, ""); print; exit }
')
[ -n "$PKG_PATH" ]
DEB="$OUT/download/busybox-static_m68k.deb"
curl -fL "$PORTS/$PKG_PATH" -o "$DEB"

DEB_ARCH=$(dpkg-deb -f "$DEB" Architecture)
if [ "$DEB_ARCH" != "m68k" ]; then
  echo "FAIL: busybox-static package architecture is '$DEB_ARCH', expected 'm68k'" >&2
  exit 1
fi

dpkg-deb -x "$DEB" "$OUT/pkg"

# Debian's usr-merge may place BusyBox in /usr/bin rather than /bin.
# Do not depend on either layout: locate the extracted executable by name.
BUSYBOX_PATH=$(find "$OUT/pkg" -type f -name busybox -print | head -n 1)
if [ -z "$BUSYBOX_PATH" ]; then
  echo "FAIL: busybox executable not found in extracted busybox-static package" >&2
  echo "Package contents:" >&2
  dpkg-deb -c "$DEB" >&2
  exit 1
fi

printf 'busybox package path: %s\n' "${BUSYBOX_PATH#$OUT/pkg}"
cp "$BUSYBOX_PATH" "$ROOT/bin/busybox"
chmod 0755 "$ROOT/bin/busybox"

for app in sh mount umount mkdir cat echo uname dmesg sleep poweroff reboot ls; do
  ln -s busybox "$ROOT/bin/$app"
done

cat > "$ROOT/init" <<'EOF'
#!/bin/sh
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev 2>/dev/null || true

echo
echo "m68kDeb installer bootstrap"
echo "=========================="
uname -a || true
echo
echo "CPU information:"
cat /proc/cpuinfo 2>/dev/null || true
echo
echo "Kernel command line:"
cat /proc/cmdline 2>/dev/null || true
echo
echo "M1.3 bootstrap reached userspace."
echo "Dropping to rescue shell; installer core arrives in later M1 milestones."
exec /bin/sh
EOF
chmod 0755 "$ROOT/init"

(
  cd "$ROOT"
  find . -print | cpio -o -H newc 2>/dev/null | gzip -9
) > "$OUT/initramfs-m68kdeb.gz"

sha256sum "$DEB" > "$OUT/busybox-static.SHA256"
sha256sum "$OUT/initramfs-m68kdeb.gz" > "$OUT/SHA256SUMS"
printf '%s\n' "$PORTS/$PKG_PATH" > "$OUT/BUSYBOX_SOURCE_URL"

printf 'initramfs: %s\n' "$OUT/initramfs-m68kdeb.gz"
cat "$OUT/SHA256SUMS"
