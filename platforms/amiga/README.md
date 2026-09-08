# Amiga platform

Amiga is the first implementation target for m68kDeb.

## Bootstrap contract

m68kDeb boots Linux from AmigaOS using `amiboot`. The bootstrap bundle contains:

- historical Debian `amiboot 5.6`, verified by pinned SHA-256;
- uncompressed `vmlinux-m68k-amiga` used by the launch scripts;
- compressed `vmlinuz-m68k-amiga` retained for preservation/future bootstrap variants;
- an m68kDeb installer initramfs;
- Amiga shell launch scripts;
- checksums and provenance metadata.

The initial launch path is intentionally conservative: copy the bundle to an AmigaOS-accessible filesystem and start Linux from there.

## Initial boot command

The reference command is:

```text
amiboot -d -k vmlinux-m68k-amiga -r initramfs-m68kdeb.gz root=/dev/ram video=pal
```

For early qualification, serial output may be added with:

```text
console=ttyS0,9600n8
```

`video=ntsc` is used for NTSC profiles. Other graphics backends are separate qualification targets rather than assumed defaults.

## Initial qualification profiles

The first emulator targets are:

- A3000 / 68030 + MMU
- A4000 / 68040 + MMU
- A1200 with 68030 accelerator + MMU
- A1200/A4000 with 68060 accelerator
- 68020 + MMU/PMMU reference profile when emulator support is adequate

The 68020+MMU profile remains a supported project minimum even if it is not the recommended installation experience.

## Qualification status

M1.2 bootstrap contract: complete.

M1.3 build/static artifact qualification: PASS. The qualified baseline is documented in `docs/M1_3_BUILD_QUALIFICATION.md`.

Amiga runtime boot: UNVERIFIED. M1.3a will attempt the first reproducible runtime path with FS-UAE and AROS Kickstart. A runtime PASS requires observed Linux execution and the m68kDeb userspace marker; successful artifact construction alone is insufficient.
