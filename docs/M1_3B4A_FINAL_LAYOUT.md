# M1.3b.4a — final kernel image layout

M1.3b.4a is the last reversible step before privileged AROS takeover. It proves that the Linux 7.2 Amiga kernel can be transformed from its on-disk ELF32 image into the contiguous physical-style image layout expected by early m68k boot code, with the bootinfo record list placed immediately after the loaded kernel image.

This sub-milestone deliberately does **not** disable interrupts, caches or the MMU and does not jump to Linux. Those irreversible operations belong to M1.3b.4b after this layout is qualified.

## Linux 7.2 contract

The exact Linux 7.2 m68k sources establish the relevant contract:

- `vmlinux-std.lds` links the normal MMU kernel from address `0x1000` and uses `ENTRY(_start)`.
- The ELF image contains `PT_LOAD` segments; file bytes and zero-filled `p_memsz - p_filesz` tails must be materialized before entry.
- `_end` is aligned at the end of the final load segment.
- `head.S` states that the bootinfo structure is located directly after the kernel and uses that position when locating `BI_LAST` and the first free page.

For the qualification probe, the loaded image span is therefore the range from the lowest `PT_LOAD.p_vaddr` through the highest `PT_LOAD.p_vaddr + p_memsz`. The probe allocates a page-aligned destination, copies every `PT_LOAD` segment to its virtual-address-relative offset, zeros BSS/tails, and places bootinfo at `layout_base + image_span` with no gap.

## Required invariants

A PASS requires:

- ELF32, big-endian, m68k validation succeeds.
- At least one `PT_LOAD` segment exists.
- Every file-backed segment range is inside the source file.
- Every materialized segment range is inside the allocated destination span.
- The ELF entry lies inside a loadable segment.
- Destination base is 4096-byte aligned.
- Loaded entry address is `layout_base + (e_entry - min_vaddr)`.
- Bootinfo starts exactly at `layout_base + image_span`.
- Bootinfo remains self-validating and ends in `BI_LAST`.
- Initramfs is represented by `BI_RAMDISK`.
- The loader returns to AROS with `handoff=NOT_ATTEMPTED`.

## Runtime profile

Qualification keeps the known-good public CI profile: matching pinned AROS m68k ROM/system, generic A1200, explicit 68030 + MMU, `uae_cpu_speed=real`, 64 MiB Zorro III FastRAM, single DH0 directory hard drive, Linux 7.2.4, and the m68kDeb initramfs. No proprietary ROM or accelerator firmware is used.

## PASS markers

The runtime report must contain at least:

- `M68KDEB_LAYOUT_START`
- `kernel_entry=`
- `layout_base=`
- `layout_image_bytes=`
- `layout_entry_addr=`
- `layout_bootinfo_addr=`
- `layout_bootinfo_immediate=1`
- `M68KDEB_FINAL_LAYOUT_OK`
- `handoff=NOT_ATTEMPTED`

## Explicit non-claims

M1.3b.4a does not prove AROS quiesce, supervisor takeover, interrupt masking, DMA shutdown, cache/MMU shutdown, kernel entry execution, Linux early boot, initramfs execution, or physical Amiga hardware support.

M1.3b.4b is permitted to attempt the first irreversible jump only after this layout probe is green.