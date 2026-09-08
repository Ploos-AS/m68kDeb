# M1.2 — Amiga bootstrap

## Purpose

M1.2 defines the reproducible handoff from AmigaOS to the m68kDeb Linux installer.

## Required bundle

A generated bootstrap directory must contain:

```text
m68kdeb-amiga/
├── amiboot
├── vmlinuz-m68k-amiga
├── initramfs-m68kdeb.gz
├── Start-m68kDeb
├── Start-m68kDeb-Serial
├── SHA256SUMS
└── PROVENANCE.txt
```

The repository does not redistribute `amiboot` or a Linux kernel merely by naming these files. The build/packaging step must fetch or consume explicitly supplied, provenance-recorded inputs and verify expected hashes where available.

## Launch contract

Normal PAL reference:

```text
amiboot -d -k vmlinuz-m68k-amiga -r initramfs-m68kdeb.gz root=/dev/ram video=pal
```

Serial-debug reference:

```text
amiboot -d -k vmlinuz-m68k-amiga -r initramfs-m68kdeb.gz root=/dev/ram video=pal console=ttyS0,9600n8
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

`scripts/check-amiga-bootstrap.sh DIR` validates the bundle shape, executable launch scripts, kernel/initramfs presence, checksums file and provenance file. It deliberately does not claim the contents boot.

## Next runtime milestone

The next Amiga milestone is to acquire/build a current Amiga-capable m68k kernel and installer initramfs, assemble the bundle, and boot it visibly in FS-UAE with serial logging available for diagnosis.
