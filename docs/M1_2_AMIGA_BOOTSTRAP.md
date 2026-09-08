# M1.2 — Amiga bootstrap

## Purpose

M1.2 defines the reproducible handoff from AmigaOS to the m68kDeb Linux installer.

## Required bundle

A generated bootstrap directory must contain:

```text
m68kdeb-amiga/
├── amiboot
├── vmlinux-m68k-amiga
├── vmlinuz-m68k-amiga
├── initramfs-m68kdeb.gz
├── Start-m68kDeb
├── Start-m68kDeb-Serial
├── SHA256SUMS
└── PROVENANCE.txt
```

`amiboot` is fetched as the historical Debian-distributed `amiboot 5.6` binary and verified against a pinned SHA-256. Linux 7.2.4 source is likewise fetched from kernel.org and verified against a pinned SHA-256 before build.

Both uncompressed and compressed kernels are retained. The launch contract uses the uncompressed `vmlinux-m68k-amiga`, because `amiboot 5.6` requires an uncompressed kernel. `vmlinuz-m68k-amiga` is retained for preservation and possible future bootstrap variants.

## Launch contract

Normal PAL reference:

```text
amiboot -d -k vmlinux-m68k-amiga -r initramfs-m68kdeb.gz root=/dev/ram video=pal
```

Serial-debug reference:

```text
amiboot -d -k vmlinux-m68k-amiga -r initramfs-m68kdeb.gz root=/dev/ram video=pal console=ttyS0,9600n8
```

NTSC profiles substitute `video=ntsc`.

## Design rules

1. AmigaOS remains only the bootstrap environment; the installer itself runs under Linux.
2. Kernel, initramfs and bootstrap utility are independent artifacts with independent provenance.
3. No assumption is made that every graphics card works with the chipset-video launch script.
4. Serial console is retained as the qualification fallback.
5. The first runtime qualification target is an MMU-equipped 68030-class Amiga profile; 68020+MMU remains in the support contract and will receive its own qualification.
6. Runtime PASS requires observed Linux kernel execution, not merely successful invocation of `amiboot`.

## Static qualification

`scripts/check-amiga-bootstrap.sh DIR` validates the bundle shape, executable launch scripts, uncompressed launch kernel, initramfs, checksums and provenance. It deliberately does not claim that the bundle boots.

M1.3 build/static qualification is recorded separately in `docs/M1_3_BUILD_QUALIFICATION.md`.

## Next runtime milestone

The next Amiga milestone is M1.3a: boot the qualified bundle through FS-UAE using a redistributable AROS Kickstart environment, observe Linux execution and the `M1.3 bootstrap reached userspace.` marker, and retain serial/log evidence. Proprietary Kickstart ROMs are not part of public CI.
