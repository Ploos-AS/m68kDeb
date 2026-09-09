/*
 * m68kDeb M1.3b.4b.3 isolated 68030 supervisor boundary stub.
 *
 * This object is intentionally NOT linked into the loader yet.  It exists to
 * qualify the m68k assembler/toolchain, the takeover-request ABI offsets, and
 * the first privileged boundary operation in isolation.
 *
 * If called, the caller MUST already be in supervisor mode.  The stub masks
 * maskable interrupts, restores the original SR, and returns.  It does not
 * touch the MMU, caches, DMA, scheduler state, kernel image, bootinfo, or jump
 * to Linux.  Later 4b milestones will replace this reversible probe with the
 * non-returning commit path after runtime qualification.
 */

    .text
    .align  2
    .globl  m68kdeb_takeover_68030_supervisor_probe
    .type   m68kdeb_takeover_68030_supervisor_probe,@function

/* struct m68kdeb_takeover_request offsets: all fields are ULONG. */
    .equ REQ_LAYOUT_BASE,      0
    .equ REQ_LAYOUT_BYTES,     4
    .equ REQ_ENTRY_ADDR,       8
    .equ REQ_BOOTINFO_ADDR,   12
    .equ REQ_BOOTINFO_BYTES,  16
    .equ REQ_INITRAMFS_ADDR,  20
    .equ REQ_INITRAMFS_BYTES, 24
    .equ REQ_CPU,             28
    .equ REQ_MMU,             32
    .equ REQ_BYTES,           36

m68kdeb_takeover_68030_supervisor_probe:
    /* GCC m68k ABI: first argument is at 4(sp). */
    move.l  4(%sp),%a0
    tst.l   %a0
    beq.s   .Lbad

    /* Touch the fields whose offsets are required by the eventual commit. */
    move.l  REQ_ENTRY_ADDR(%a0),%a1
    move.l  REQ_BOOTINFO_ADDR(%a0),%d1
    move.l  REQ_INITRAMFS_ADDR(%a0),%d2
    tst.l   %a1
    beq.s   .Lbad
    tst.l   %d1
    beq.s   .Lbad
    tst.l   %d2
    beq.s   .Lbad

    /* Privileged boundary probe: caller must already be supervisor. */
    move.w  %sr,%d0
    move.w  %d0,%d1
    ori.w   #0x0700,%sr
    move.w  %d0,%sr

    /* Return 0; the original SR has been restored. */
    moveq   #0,%d0
    rts

.Lbad:
    moveq   #1,%d0
    rts

    .size m68kdeb_takeover_68030_supervisor_probe,.-m68kdeb_takeover_68030_supervisor_probe
