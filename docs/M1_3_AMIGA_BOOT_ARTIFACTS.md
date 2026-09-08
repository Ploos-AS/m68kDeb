# M1.3 — Amiga boot artifacts

## Goal

Produce a current, provenance-recorded Linux/m68k Amiga kernel and a minimal m68kDeb installer initramfs, then assemble them into the M1.2 `amiboot` bundle contract.

## Kernel baseline

The builder pins Linux `7.2.4` and starts from upstream `arch/m68k/configs/amiga_defconfig`.

The source tarball is fetched from kernel.org and verified before extraction against the pinned SHA-256:

```text
01710ee01737dac492f1bae52becd057e08d20d11589089aa06accff415c28dd  linux-7.2.4.tar.xz
```

The build enables the installer-critical built-ins required for an initramfs bootstrap while retaining upstream Amiga configuration as the compatibility baseline.

Outputs:

- `out/kernel-amiga/vmlinux-m68k-amiga` — uncompressed kernel used by the `amiboot 5.6` launch path.
- `out/kernel-amiga/vmlinuz-m68k-amiga` — compressed `vmlinux.gz`, retained for preservation/future bootstrap variants.

## Amiga boot loader baseline

The bundle uses Debian's historical `amiboot 5.6` binary. It is fetched from the Debian Woody archive and verified against the pinned SHA-256:

```text
8aae977164de07dea64857a6aa6a47c2bf8cc02dc3bde0052c42975cb7e0df9d  amiboot
```

Because `amiboot 5.6` requires an uncompressed kernel, both launch scripts use `vmlinux-m68k-amiga`.

## Initramfs baseline

The initramfs builder resolves the current `busybox-static` m68k package from Debian Ports `unstable`, verifies that the package architecture is `m68k`, extracts BusyBox without executing it on the host, and constructs a small gzip-compressed `newc` initramfs.

The package used by the qualified M1.3 run had SHA-256:

```text
4fb7f78eb996019f2a484ec6b4ec5b5ccf2103961c18d678eb1091ff85ed9bc5  busybox-static_m68k.deb
```

The M1.3 `/init` script mounts proc/sys/devtmpfs, prints CPU/kernel-command-line diagnostics, emits the `M1.3 bootstrap reached userspace.` marker and enters a rescue shell.

This is intentionally not the finished installer UI. Its purpose is to prove the complete AmigaOS -> amiboot -> Linux kernel -> m68k userspace path.

## CI qualification boundary

The `M1.3 Amiga boot artifacts` workflow qualifies:

- pinned Linux source integrity;
- cross-compilation of upstream Linux for Amiga/m68k;
- creation of an m68k BusyBox initramfs;
- pinned `amiboot 5.6` integrity;
- creation of the M1.2 bundle shape;
- bundle checksums and provenance;
- static bootstrap validation.

Run #12 (`34259220026`) on commit `9e307ec9b58fac5656b39679856058920a870f1e` completed successfully and is the M1.3 build/static qualification baseline. Full evidence is recorded in `docs/M1_3_BUILD_QUALIFICATION.md`.

It does **not** qualify Amiga runtime boot. Runtime PASS requires observed Linux execution and the `M1.3 bootstrap reached userspace.` output.

## Runtime target — M1.3a

First reference runtime target:

- FS-UAE;
- redistributable AROS Kickstart for public/reproducible CI;
- Amiga 68030-class machine with MMU;
- PAL chipset video;
- enough Fast RAM for the current kernel/initramfs;
- normal console first, serial-debug launch retained as fallback.

The intended automated chain is:

```text
GitHub Actions -> Xvfb -> FS-UAE -> AROS Kickstart -> amiboot -> Linux 7.2.4 -> m68kDeb initramfs -> PASS marker
```

AROS compatibility with this exact bootstrap path remains UNVERIFIED until M1.3a executes it. CI must not silently fall back to a proprietary Kickstart ROM if AROS proves incompatible.

68020+MMU remains an explicit support target and will be qualified separately after the first 68030 path is proven.
