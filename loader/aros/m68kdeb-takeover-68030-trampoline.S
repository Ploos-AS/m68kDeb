/*
 * m68kDeb M1.3b.4b.5d MC68030 physical-transition trampoline probe.
 *
 * Compile/static qualification only.  This object MUST NOT be linked into the
 * real loader or executed yet.  It establishes a tiny relocation-free,
 * position-independent code blob that can later be copied to a qualified
 * physical==logical address before translation is disabled.
 *
 * No cache/MMU manipulation and no Linux jump are performed here.
 */

    .text
    .align  2
    .globl  m68kdeb_takeover_68030_trampoline_start
    .globl  m68kdeb_takeover_68030_trampoline_end
    .globl  m68kdeb_takeover_68030_trampoline_probe
    .type   m68kdeb_takeover_68030_trampoline_probe,@function

m68kdeb_takeover_68030_trampoline_start:
m68kdeb_takeover_68030_trampoline_probe:
    /* Deliberately position-independent and side-effect free for 5d. */
    moveq   #0,%d0
    rts
m68kdeb_takeover_68030_trampoline_end:

    .size m68kdeb_takeover_68030_trampoline_probe,.-m68kdeb_takeover_68030_trampoline_probe
