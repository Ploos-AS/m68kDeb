#!/bin/sh
set -eu

LINUX_VERSION=${LINUX_VERSION:-7.2.4}
LINUX_SHA256=${LINUX_SHA256:-01710ee01737dac492f1bae52becd057e08d20d11589089aa06accff415c28dd}
JOBS=${JOBS:-2}
OUT=${OUT:-out/kernel-amiga}
WORK=${WORK:-out/kernel-work}
TARBALL="$WORK/linux-$LINUX_VERSION.tar.xz"
SRC="$WORK/linux-$LINUX_VERSION"
URL="https://cdn.kernel.org/pub/linux/kernel/v7.x/linux-$LINUX_VERSION.tar.xz"

command -v curl >/dev/null
command -v tar >/dev/null
command -v make >/dev/null
command -v m68k-linux-gnu-gcc >/dev/null
command -v sha256sum >/dev/null
command -v python3 >/dev/null

rm -rf "$WORK" "$OUT"
mkdir -p "$WORK" "$OUT"

curl -fL "$URL" -o "$TARBALL"
ACTUAL_SHA256=$(sha256sum "$TARBALL" | awk '{print $1}')
if [ "$ACTUAL_SHA256" != "$LINUX_SHA256" ]; then
  echo "FAIL: Linux source SHA-256 mismatch" >&2
  echo "expected=$LINUX_SHA256" >&2
  echo "actual=$ACTUAL_SHA256" >&2
  exit 1
fi
printf '%s  %s\n' "$ACTUAL_SHA256" "$(basename "$TARBALL")" > "$OUT/linux-source.SHA256"
tar -C "$WORK" -xf "$TARBALL"

# M1.3b.4b.6c diagnostic: make the 68030 MMU engage sequence observable on
# the existing early serial channel. H is emitted immediately before the
# mmu_engage call by upstream head.S. These additional markers isolate the
# irreversible 68030 transition without changing the mappings themselves:
#   J = entered mmu_engage_030
#   K = SRP loaded
#   L = PFLUSHA completed
#   M = immediately before TC enable
#   N = TC enable returned / next instruction fetched
python3 - "$SRC/arch/m68k/kernel/head.S" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()
needle = '''L(mmu_engage_030):
\t.chip\t68030
\tlea\t%pc@(L(mmu_engage_030_temp)),%a0
\tmovel\t#0x80000002,%a0@
\tmovel\t%a3,%a0@(4)
\tmovel\t#0x0808,%d0
\tmovec\t%d0,%cacr
\tpmove\t%a0@,%srp
\tpflusha
\t/*
\t * enable,super root enable,4096 byte pages,7 bit root index,
\t * 7 bit pointer index, 6 bit page table index.
\t */
\tmovel\t#0x82c07760,%a0@(8)
\tpmove\t%a0@(8),%tc\t\t/* enable the MMU */
\tjmp\t1f:l
'''
replacement = '''L(mmu_engage_030):
\t.chip\t68030
\tputc\t`'J'`
\tlea\t%pc@(L(mmu_engage_030_temp)),%a0
\tmovel\t#0x80000002,%a0@
\tmovel\t%a3,%a0@(4)
\tmovel\t#0x0808,%d0
\tmovec\t%d0,%cacr
\tpmove\t%a0@,%srp
\tputc\t`'K'`
\tpflusha
\tputc\t`'L'`
\t/*
\t * enable,super root enable,4096 byte pages,7 bit root index,
\t * 7 bit pointer index, 6 bit page table index.
\t */
\tmovel\t#0x82c07760,%a0@(8)
\tputc\t`'M'`
\tpmove\t%a0@(8),%tc\t\t/* enable the MMU */
\tputc\t`'N'`
\tjmp\t1f:l
'''
count = text.count(needle)
if count != 1:
    raise SystemExit(f'FAIL: expected one 68030 mmu_engage block, found {count}')
path.write_text(text.replace(needle, replacement, 1))
PY
printf '%s\n' 'H->J(entry)->K(SRP)->L(PFLUSHA)->M(pre-TC)->N(post-TC)' > "$OUT/MMU_68030_TRACE.txt"

make -C "$SRC" ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- amiga_defconfig

# Installer-critical facilities are built in. Keep the upstream Amiga
# defconfig as the compatibility baseline and only force what the m68kDeb
# bootstrap contract needs.
"$SRC/scripts/config" --file "$SRC/.config" \
  --enable BLK_DEV_INITRD \
  --enable DEVTMPFS \
  --enable DEVTMPFS_MOUNT \
  --enable TMPFS \
  --enable RD_GZIP \
  --enable PROC_FS \
  --enable SYSFS \
  --enable EXT4_FS

make -C "$SRC" ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- olddefconfig
make -C "$SRC" -j"$JOBS" ARCH=m68k CROSS_COMPILE=m68k-linux-gnu- zImage

# Keep both forms. Debian's Amiga notes recommend amiboot 5.6 as the
# reliable fallback, and 5.6 requires an uncompressed kernel image.
cp "$SRC/vmlinux" "$OUT/vmlinux-m68k-amiga"
cp "$SRC/vmlinux.gz" "$OUT/vmlinuz-m68k-amiga"
cp "$SRC/.config" "$OUT/kernel.config"
cp "$SRC/System.map" "$OUT/System.map"
printf '%s\n' "$LINUX_VERSION" > "$OUT/VERSION"
printf '%s\n' "$URL" > "$OUT/SOURCE_URL"
sha256sum \
  "$OUT/vmlinux-m68k-amiga" \
  "$OUT/vmlinuz-m68k-amiga" \
  "$OUT/kernel.config" \
  "$OUT/System.map" > "$OUT/SHA256SUMS"

printf 'kernel: %s\n' "$OUT/vmlinux-m68k-amiga"
printf 'compressed kernel: %s\n' "$OUT/vmlinuz-m68k-amiga"
cat "$OUT/SHA256SUMS"
