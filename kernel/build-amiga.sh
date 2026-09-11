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

start_marker = "L(mmu_engage_030):\n"
end_marker = "\nL(mmu_engage_cleanup):\n"
start = text.find(start_marker)
if start < 0:
    raise SystemExit('FAIL: mmu_engage_030 block start not found')
end = text.find(end_marker, start)
if end < 0:
    raise SystemExit('FAIL: mmu_engage_030 block end not found')

block = text[start:end]

def replace_block_once(old, new, label):
    global block
    count = block.count(old)
    if count != 1:
        raise SystemExit(f'FAIL: expected one {label} anchor in mmu_engage_030, found {count}')
    block = block.replace(old, new, 1)

replace_block_once(
    "L(mmu_engage_030):\n\t.chip\t68030\n",
    "L(mmu_engage_030):\n\t.chip\t68030\n\tputc\t`'J'`\n",
    "mmu_engage_030 entry",
)
replace_block_once(
    "\tmovel\t#0x0808,%d0\n\tmovec\t%d0,%cacr\n\tpmove\t%a0@,%srp\n\tpflusha\n",
    "\tmovel\t#0x0808,%d0\n\tmovec\t%d0,%cacr\n\tpmove\t%a0@,%srp\n\tputc\t`'K'`\n\tpflusha\n\tputc\t`'L'`\n",
    "first 030 SRP/PFLUSHA",
)
replace_block_once(
    "\tmovel\t#0x82c07760,%a0@(8)\n\tpmove\t%a0@(8),%tc\t/* enable the MMU */\n\tjmp\t1f:l\n",
    "\tmovel\t#0x82c07760,%a0@(8)\n\tputc\t`'M'`\n\tpmove\t%a0@(8),%tc\t/* enable the MMU */\n\tputc\t`'N'`\n\tjmp\t1f:l\n",
    "030 TC enable",
)

text = text[:start] + block + text[end:]
path.write_text(text)
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
