/*
 * m68kDeb M1.3b.4b.5e MC68030 physical-transition trampoline.
 *
 * Compile/static qualification only. This object MUST NOT be linked into the
 * real loader or executed yet. It defines the complete instruction/data span
 * that would have to reside at a qualified physical==logical address before
 * translation can safely be disabled.
 *
 * The 5e gate proves containment and relocation freedom only. It does NOT
 * prove that any AROS virtual address equals its physical address, and it does
 * NOT authorize runtime execution.
 */

    .text
    .align  2
    .globl  m68kdeb_takeover_68030_trampoline_start
    .globl  m68kdeb_takeover_68030_trampoline_entry
    .globl  m68kdeb_takeover_68030_trampoline_probe
    .globl  m68kdeb_takeover_68030_trampoline_mmu_off
    .globl  m68kdeb_takeover_68030_trampoline_post_mmu
    .globl  m68kdeb_takeover_68030_trampoline_tc_zero
    .globl  m68kdeb_takeover_68030_trampoline_end
    .type   m68kdeb_takeover_68030_trampoline_entry,@function

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

    /* 5e intentionally has no kernel jump. A runtime-capable successor will
     * replace this halt only after physical placement and entry ABI are proven.
     */
.Lhalt:
    bra.s   .Lhalt

    .align  2
m68kdeb_takeover_68030_trampoline_tc_zero:
    .long   0
m68kdeb_takeover_68030_trampoline_end:

    .size m68kdeb_takeover_68030_trampoline_entry,.-m68kdeb_takeover_68030_trampoline_entry
