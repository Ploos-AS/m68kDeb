/*
 * m68kDeb 68030 takeover boundary code.
 *
 * M1.3b.4b.3 qualifies the reversible supervisor probe.
 * M1.3b.4b.4 adds the first deliberately non-returning commit boundary.
 *
 * This object remains intentionally NOT linked into the loader.  The commit
 * function only establishes the irreversible supervisor/interrupt-mask
 * boundary; it does not yet quiesce DMA, change cache/MMU state, or jump to
 * the Linux entry point.  Those operations belong to later 4b milestones.
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
    beq.s   .Lprobe_bad

    /* Touch the fields whose offsets are required by the eventual commit. */
    move.l  REQ_ENTRY_ADDR(%a0),%a1
    move.l  REQ_BOOTINFO_ADDR(%a0),%d1
    move.l  REQ_INITRAMFS_ADDR(%a0),%d2
    tst.l   %a1
    beq.s   .Lprobe_bad
    tst.l   %d1
    beq.s   .Lprobe_bad
    tst.l   %d2
    beq.s   .Lprobe_bad

    /* Privileged boundary probe: caller must already be supervisor. */
    move.w  %sr,%d0
    move.w  %d0,%d1
    ori.w   #0x0700,%sr
    move.w  %d0,%sr

    /* Return 0; the original SR has been restored. */
    moveq   #0,%d0
    rts

.Lprobe_bad:
    moveq   #1,%d0
    rts

    .size m68kdeb_takeover_68030_supervisor_probe,.-m68kdeb_takeover_68030_supervisor_probe

    .align  2
    .globl  m68kdeb_takeover_68030_commit
    .globl  m68kdeb_takeover_68030_commit_boundary
    .type   m68kdeb_takeover_68030_commit,@function

/*
 * First irreversible boundary.
 *
 * Preconditions are enforced by m68kdeb_takeover_68030_prepare() before this
 * function may ever be linked/called.  The caller must already be executing
 * in supervisor mode.  Once SR interrupt priority is raised to 7 this path
 * intentionally never returns to AROS.
 */
m68kdeb_takeover_68030_commit:
    move.l  4(%sp),%a0
    tst.l   %a0
    beq.s   .Lcommit_halt

    /* Load and retain addresses required by the later Linux handoff stages. */
    move.l  REQ_ENTRY_ADDR(%a0),%a1
    move.l  REQ_BOOTINFO_ADDR(%a0),%a2
    move.l  REQ_INITRAMFS_ADDR(%a0),%a3

    /*
     * M1.3b.4b.4 irreversible boundary: after this instruction there is no
     * restoration of SR and no return to the operating system.
     */
m68kdeb_takeover_68030_commit_boundary:
    ori.w   #0x0700,%sr

.Lcommit_halt:
    bra.s   .Lcommit_halt

    .size m68kdeb_takeover_68030_commit,.-m68kdeb_takeover_68030_commit
