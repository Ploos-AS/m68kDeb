# M1.3b.3 — initramfs + BI_RAMDISK contract

M1.3b.3 extends the AROS-native `m68kdeb-loader` qualification path from kernel-file loading plus bootinfo construction to loading the m68kDeb initramfs and describing it to Linux with `BI_RAMDISK`.

This milestone is still deliberately pre-handoff. It must return cleanly to AROS after constructing and validating the bootinfo data. It does **not** claim Linux execution or Amiga runtime qualification.

## Linux/m68k ABI

Linux 7.2 defines `BI_RAMDISK` as machine-independent tag `0x0006`. Its payload uses the same `struct mem_info` layout as `BI_MEMCHUNK`: two big-endian 32-bit values, address followed by size.

The loader must therefore emit:

- `BI_RAMDISK`
- ramdisk address as a 32-bit big-endian value
- ramdisk size as a 32-bit big-endian value

The final handoff implementation must ensure this address is a valid physical address from the kernel's point of view. M1.3b.3 only proves the AROS-side loading, record construction, bounds checks, and self-validation.

## Runtime contract

The qualification profile remains the known-good public CI environment:

- Ubuntu 24.04 GitHub runner
- FS-UAE
- matching pinned AROS m68k nightly ROM/system
- generic A1200 profile
- explicit 68030 + MMU
- `uae_cpu_speed = real`
- 64 MiB Zorro III FastRAM
- single DH0 directory hard drive
- no proprietary Commodore/Amiga ROM
- no accelerator ROM

The runtime directory contains:

- `m68kdeb-loader`
- uncompressed Linux/m68k `vmlinux-m68k-amiga`
- `initramfs-m68kdeb.gz`

## Loader interface

M1.3b.2 compatibility must be retained. Kernel-only bootinfo construction remains valid.

M1.3b.3 adds an initramfs argument and optional Amiga model ID. The implementation should distinguish the existing numeric model argument from an initramfs path so the M1.3b.1 and M1.3b.2 regression workflows continue to work unchanged.

## Required evidence

A PASS must prove all of the following before returning to AROS:

- AROS startup marker reached
- loader invocation reached
- Linux ELF32 big-endian m68k kernel loaded
- matching Amiga bootinfo ABI found in the kernel image
- initramfs file loaded completely
- `BI_RAMDISK` emitted with non-zero address and size
- generated bootinfo list self-validates
- `BI_LAST` is the final record
- loader reports `handoff=NOT_ATTEMPTED`
- loader returns to AROS

Suggested runtime markers:

- `M68KDEB_KERNEL_LOADED`
- `M68KDEB_INITRAMFS_LOADED`
- `bootinfo_ramdisk_addr=`
- `bootinfo_ramdisk_bytes=`
- `M68KDEB_BOOTINFO_BUILD_OK`
- `M68KDEB_INITRAMFS_BOOTINFO_OK`

## Explicit non-claims

M1.3b.3 does not prove:

- correct final physical kernel placement
- correct final physical initramfs placement
- bootinfo placement immediately after the kernel
- AROS quiesce/takeover correctness
- cache/MMU/interrupt shutdown correctness
- kernel entry transfer
- Linux early boot
- initramfs userspace execution
- physical Amiga hardware support

Those are later M1.3b milestones.