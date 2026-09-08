# M1.3 — Amiga boot artifacts

## Goal

Produce a current, provenance-recorded Linux/m68k Amiga kernel and a minimal m68kDeb installer initramfs, then assemble them into the M1.2 `amiboot` bundle contract.

## Kernel baseline

The builder pins Linux `7.2.4`, the current stable release at M1.3 implementation time, and starts from upstream `arch/m68k/configs/amiga_defconfig`.

The build enables the installer-critical built-ins required for an initramfs bootstrap while retaining upstream Amiga configuration as the compatibility baseline.

Output:

`out/kernel-amiga/vmlinuz-m68k-amiga`

The m68k architecture's `zImage` target produces `vmlinux.gz`; m68kDeb renames that artifact for the bootstrap bundle.

## Initramfs baseline

The initramfs builder resolves the current `busybox-static` m68k package from Debian Ports `unstable`, extracts the m68k BusyBox binary without executing it on the host, and constructs a small gzip-compressed `newc` initramfs.

The M1.3 `/init` script mounts proc/sys/devtmpfs, prints CPU/kernel-command-line diagnostics, emits the M1.3 userspace-reached marker and enters a rescue shell.

This is intentionally not the finished installer UI. Its purpose is to prove the complete AmigaOS -> amiboot -> Linux kernel -> m68k userspace path.

## CI qualification boundary

The `M1.3 Amiga boot artifacts` workflow qualifies:

- cross-compilation of upstream Linux for Amiga/m68k;
- creation of an m68k BusyBox initramfs;
- creation of the M1.2 bundle shape;
- hashes and provenance;
- static bootstrap validation.

It does **not** qualify Amiga runtime boot. Runtime PASS requires a visible FS-UAE run and observed `M1.3 bootstrap reached userspace.` output.

## Runtime target

First reference runtime target:

- Amiga 68030-class machine with MMU;
- PAL chipset video;
- enough Fast RAM for a current kernel/initramfs;
- normal console first, serial-debug launch retained as fallback.

68020+MMU remains an explicit support target and will be qualified separately after the first 68030 path is proven.
