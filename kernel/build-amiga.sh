#!/bin/sh
set -eu

# CI retrigger: validate index-based 68030 MMU instrumentation in 6b runtime.
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

entry_anchor = "L(mmu_engage_030):\n\t.chip\t68030\n"
entry_pos = block.find(entry_anchor)
if entry_pos < 0:
    raise SystemExit('FAIL: mmu_engage_030 entry anchor not found')
entry_repl = "L(mmu_engage_030):\n\t.chip\t68030\n\tputc\t'J'\n"
block = block[:entry_pos] + entry_repl + block[entry_pos + len(entry_anchor):]

srp_anchor = "\tpmove\t%a0@,%srp\n\tpflusha\n"
srp_pos = block.find(srp_anchor)
if srp_pos < 0:
    raise SystemExit('FAIL: first 030 SRP/PFLUSHA anchor not found')
srp_repl = "\tpmove\t%a0@,%srp\n\tputc\t'K'\n\tpflusha\n\tputc\t'L'\n"
block = block[:srp_pos] + srp_repl + block[srp_pos + len(srp_anchor):]

tc_anchor = "\tpmove\t%a0@(8),%tc\t/* enable the MMU */\n\tjmp\t1f:l\n"
tc_pos = block.find(tc_anchor, srp_pos + len(srp_repl))
if tc_pos < 0:
    raise SystemExit('FAIL: 030 TC enable anchor not found')
tc_repl = "\tputc\t'M'\n\tpmove\t%a0@(8),%tc\t/* enable the MMU */\n\tputc\t'N'\n\tjmp\t1f:l\n"
block = block[:tc_pos] + tc_repl + block[tc_pos + len(tc_anchor):]

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
