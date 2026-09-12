#!/bin/sh
set -eu

# M1.3b.4b.6b runtime diagnostics for the 68030 Linux MMU handoff.
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

# Diagnostic A/B test: suppress the *68030* Amiga Zorro III TT1 mapping.
# The previous experiment accidentally matched the 68040 _PAGE_NOCACHE_S
# branch.  The 68030 branch is _PAGE_NOCACHE030.  This is diagnostic-only;
# production must retain the upstream mapping unless evidence says otherwise.
python3 - "$SRC/arch/m68k/kernel/head.S" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()
anchor = "\tmmu_map_tt\t#1,#0x40000000,#0x20000000,#_PAGE_NOCACHE030\n"
count = text.count(anchor)
if count != 1:
    raise SystemExit(f'FAIL: expected one 68030 Amiga Zorro III TT1 mapping, found {count}')
text = text.replace(
    anchor,
    "\t/* m68kDeb 6b.9 diagnostic: 68030 Zorro III TT1 suppressed */\n",
    1,
)
path.write_text(text)
PY
printf '%s\n' 'disabled: Amiga 68030 Zorro III TT1 0x40000000/0x20000000' > "$OUT/MMU_68030_TT1_AB_TEST.txt"

# Instrument the 68030 MMU engage sequence. Everything below runs while the
# MMU is still disabled until M.  S dumps the exact 64-bit SRP descriptor
# image and the TC value Linux is about to load.  R walks the temporary mapping
# for the linked/logical long-jump target.  P independently walks the mapping
# for the current PC-relative/physical long-jump target.  The latter identity
# mapping is critical: immediately after PMOVE enables TC the CPU must fetch
# the JMP instruction at the still-physical PC before the long jump can switch
# execution to its linked/logical address.
#   S <srp-hi> <srp-lo> <tc>
#   R <logical-pc> <root-raw> <root-base> <ptr-raw> <ptr-base> <pte-raw> <pte-base>
#   P <physical-pc> <root-raw> <root-base> <ptr-raw> <ptr-base> <pte-raw> <pte-base>
# The indices are those encoded by TC=0x82c07760: 7/7/6 bits, 4K pages.
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

tc_anchor = "\tmovel\t#0x82c07760,%a0@(8)\n\tpmove\t%a0@(8),%tc\t/* enable the MMU */\n\tjmp\t1f:l\n1:\tmovel\t%a2,%a0@(4)\n"
tc_pos = block.find(tc_anchor, srp_pos + len(srp_repl))
if tc_pos < 0:
    raise SystemExit('FAIL: 030 TC enable/long-jump anchor not found')

tc_repl = (
    "\tputc\t'T'\n"
    "\tputn\t%a3\n"
    "\tputn\t%a2\n"
    "\tpmove\t%tt1,%a0@(8)\n"
    "\tmovel\t%a0@(8),%d0\n"
    "\tputn\t%d0\n"
    # Dump the exact SRP image Linux loaded and the pending TC value.
    "\tputc\t'S'\n"
    "\tmovel\t%a0@,%d0\n"
    "\tputn\t%d0\n"
    "\tmovel\t%a0@(4),%d0\n"
    "\tputn\t%d0\n"
    "\tmovel\t#0x82c07760,%d0\n"
    "\tputn\t%d0\n"
    # Walk the mapping for the linked/logical post-TC target.
    "\tputc\t'R'\n"
    "\tmovel\t#1f,%a1\n"
    "\tputn\t%a1\n"
    "\tmovel\t%a1,%d0\n"
    "\tmoveq\t#ROOT_INDEX_SHIFT,%d1\n"
    "\tlsrl\t%d1,%d0\n"
    "\tandl\t#ROOT_TABLE_SIZE-1,%d0\n"
    "\tmovel\t%a3@(%d0*4),%d1\n"
    "\tputn\t%d1\n"
    "\tandw\t#-ROOT_TABLE_SIZE,%d1\n"
    "\tputn\t%d1\n"
    "\tmovel\t%d1,%a1\n"
    "\tmovel\t#1f,%d0\n"
    "\tmoveq\t#PTR_INDEX_SHIFT,%d1\n"
    "\tlsrl\t%d1,%d0\n"
    "\tandl\t#PTR_TABLE_SIZE-1,%d0\n"
    "\tmovel\t%a1@(%d0*4),%d1\n"
    "\tputn\t%d1\n"
    "\tandw\t#-PTR_TABLE_SIZE,%d1\n"
    "\tputn\t%d1\n"
    "\tmovel\t%d1,%a1\n"
    "\tmovel\t#1f,%d0\n"
    "\tmoveq\t#PAGE_INDEX_SHIFT,%d1\n"
    "\tlsrl\t%d1,%d0\n"
    "\tandl\t#PAGE_TABLE_SIZE-1,%d0\n"
    "\tmovel\t%a1@(%d0*4),%d1\n"
    "\tputn\t%d1\n"
    "\tmovel\t%d1,%d0\n"
    "\tandl\t#0xfffff000,%d0\n"
    "\tputn\t%d0\n"
    # Walk the second alias created by mmu_temp_map: the current physical
    # address must identity-map so the first instruction fetch after PMOVE TC
    # can reach the mandatory long JMP.  PC-relative LEA gives that runtime
    # physical address while the MMU is still disabled.
    "\tputc\t'P'\n"
    "\tlea\t%pc@(1f),%a1\n"
    "\tputn\t%a1\n"
    "\tmovel\t%a1,%d0\n"
    "\tmoveq\t#ROOT_INDEX_SHIFT,%d1\n"
    "\tlsrl\t%d1,%d0\n"
    "\tandl\t#ROOT_TABLE_SIZE-1,%d0\n"
    "\tmovel\t%a3@(%d0*4),%d1\n"
    "\tputn\t%d1\n"
    "\tandw\t#-ROOT_TABLE_SIZE,%d1\n"
    "\tputn\t%d1\n"
    "\tmovel\t%d1,%a1\n"
    "\tlea\t%pc@(1f),%a0\n"
    "\tmovel\t%a0,%d0\n"
    "\tmoveq\t#PTR_INDEX_SHIFT,%d1\n"
    "\tlsrl\t%d1,%d0\n"
    "\tandl\t#PTR_TABLE_SIZE-1,%d0\n"
    "\tmovel\t%a1@(%d0*4),%d1\n"
    "\tputn\t%d1\n"
    "\tandw\t#-PTR_TABLE_SIZE,%d1\n"
    "\tputn\t%d1\n"
    "\tmovel\t%d1,%a1\n"
    "\tmovel\t%a0,%d0\n"
    "\tmoveq\t#PAGE_INDEX_SHIFT,%d1\n"
    "\tlsrl\t%d1,%d0\n"
    "\tandl\t#PAGE_TABLE_SIZE-1,%d0\n"
    "\tmovel\t%a1@(%d0*4),%d1\n"
    "\tputn\t%d1\n"
    "\tmovel\t%d1,%d0\n"
    "\tandl\t#0xfffff000,%d0\n"
    "\tputn\t%d0\n"
    # Restore the mmu_engage temporary descriptor pointer after diagnostics.
    "\tlea\t%pc@(L(mmu_engage_030_temp)),%a0\n"
    "\tmovel\t#0x82c07760,%a0@(8)\n"
    "\tputc\t'M'\n"
    "\tpmove\t%a0@(8),%tc\t/* enable the MMU */\n"
    "\tjmp\t1f:l\n"
    "1:\tputc\t'N'\n"
    "\tmovel\t%a2,%a0@(4)\n"
)
block = block[:tc_pos] + tc_repl + block[tc_pos + len(tc_anchor):]

text = text[:start] + block + text[end:]
path.write_text(text)
PY
printf '%s\n' 'H->J(entry)->K(SRP)->L(PFLUSHA)->T(a3,a2,TT1)->S(srp-hi,srp-lo,tc)->R(logical-pc,root-raw,root-base,ptr-raw,ptr-base,pte-raw,pte-base)->P(physical-pc,root-raw,root-base,ptr-raw,ptr-base,pte-raw,pte-base)->M(pre-TC)->long-jump->N' > "$OUT/MMU_68030_TRACE.txt"

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
