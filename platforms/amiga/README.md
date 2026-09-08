# Amiga platform

Amiga is the first implementation target for m68kDeb.

## Bootstrap contract

m68kDeb boots Linux from AmigaOS using `amiboot`. The bootstrap bundle is expected to contain:

- `amiboot`
- an Amiga-capable Linux/m68k kernel
- an m68kDeb installer initramfs
- one or more Amiga shell launch scripts
- checksums and provenance metadata

The initial launch path is intentionally conservative: copy the bundle to an AmigaOS-accessible filesystem and start Linux from there.

## Initial boot command

The reference command shape is:

```text
amiboot -d -k vmlinuz-m68k-amiga -r initramfs-m68kdeb.gz root=/dev/ram video=pal
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

## M1.2 exit criteria

M1.2 is complete when the repository contains a reproducible Amiga bootstrap bundle definition and static validation for its required artifacts and launch scripts. Runtime boot qualification is a later M1 sub-milestone and must be recorded separately.
