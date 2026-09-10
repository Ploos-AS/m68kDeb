/*
 * m68kDeb M1.3b.4b.5e MC68030 physical-transition trampoline.
 *
 * Compile/static qualification only for the irreversible entry. Later
 * milestones may execute only the explicitly reversible probe entry below.
 * The real transition entry remains forbidden until separately qualified.
 */

    .text
    .align  2
    .globl  m68kdeb_takeover_68030_trampoline_start
    .globl  m68kdeb_takeover_68030_trampoline_entry
    .globl  m68kdeb_takeover_68030_trampoline_probe
    .globl  m68kdeb_takeover_68030_trampoline_mmu_off
    .globl  m68kdeb_takeover_68030_trampoline_post_mmu
    .globl  m68kdeb_takeover_68030_trampoline_tc_zero
    .globl  m68kdeb_takeover_68030_trampoline_reversible_probe
    .globl  m68kdeb_takeover_68030_trampoline_end
    .type   m68kdeb_takeover_68030_trampoline_entry,@function
    .type   m68kdeb_takeover_68030_trampoline_reversible_probe,@function

m68kdeb_takeover_68030_trampoline_start:
m68kdeb_takeover_68030_trampoline_entry:
/* Keep the 5d probe name as an alias while 5e strengthens the blob. */
m68kdeb_takeover_68030_trampoline_probe:
    /* Disable MC68030 caches before the translation-state transition. */
    moveq   #0,%d0
    movec   %d0,%cacr

    /* TC=0 expresses the intended translation-disable operation. */
    lea     m68kdeb_takeover_68030_trampoline_tc_zero(%pc),%a0
m68kdeb_takeover_68030_trampoline_mmu_off:
    pmove   (%a0),%tc

m68kdeb_takeover_68030_trampoline_post_mmu:
    /* Every instruction fetched from here must remain physically reachable. */
    pflusha

    /* 5e intentionally has no kernel jump. */
.Lhalt:
    bra.s   .Lhalt

    .align  2
m68kdeb_takeover_68030_trampoline_tc_zero:
    .long   0

/*
 * M1.3b.4b.5s reversible execution probe.
 * This entry is deliberately isolated after the transition data and performs
 * no privileged operation, cache/MMU mutation, external reference or branch
 * into the real transition path. A copied blob may call this offset and must
 * return to its caller via RTS.
 */
m68kdeb_takeover_68030_trampoline_reversible_probe:
    moveq   #0,%d0
    rts
m68kdeb_takeover_68030_trampoline_end:

    .size m68kdeb_takeover_68030_trampoline_entry,.-m68kdeb_takeover_68030_trampoline_entry
    .size m68kdeb_takeover_68030_trampoline_reversible_probe,.-m68kdeb_takeover_68030_trampoline_reversible_probe
