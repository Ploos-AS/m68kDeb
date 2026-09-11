/*
 * M1.3b.4b.5aa CI-only MC68030 Linux-entry transition trampoline.
 *
 * This file is deliberately separate from the production/5z containment
 * trampoline.  It is assembled into a copied identity-qualified page only by
 * the 5aa qualification workflow.  Entry register contract:
 *   a0 = takeover request (scratch after entry)
 *   a1 = physical/logical Linux entry address
 *   a2 = bootinfo address
 *   a3 = initramfs address
 *
 * The sequence is intentionally the already-qualified 5z transition followed
 * by one new operation: an unconditional transfer to the actual Linux entry.
 */

    .text
    .align  2
    .globl  m68kdeb_5aa_trampoline_start
    .globl  m68kdeb_5aa_trampoline_entry
    .globl  m68kdeb_5aa_trampoline_mmu_off
    .globl  m68kdeb_5aa_trampoline_post_mmu
    .globl  m68kdeb_5aa_trampoline_linux_jump
    .globl  m68kdeb_5aa_trampoline_tc_zero
    .globl  m68kdeb_5aa_trampoline_abi_probe
    .globl  m68kdeb_5aa_trampoline_reversible_probe
    .globl  m68kdeb_5aa_trampoline_end

m68kdeb_5aa_trampoline_start:
m68kdeb_5aa_trampoline_entry:
    moveq   #0,%d0
    movec   %d0,%cacr

    lea     m68kdeb_5aa_trampoline_tc_zero(%pc),%a0
m68kdeb_5aa_trampoline_mmu_off:
    pmove   (%a0),%tc

m68kdeb_5aa_trampoline_post_mmu:
    pflusha

/* The only new irreversible operation relative to qualified 5z. */
m68kdeb_5aa_trampoline_linux_jump:
    jmp     (%a1)

    .align  2
m68kdeb_5aa_trampoline_tc_zero:
    .long   0

/* Reversible ABI preflight entry; never falls through into Linux. */
m68kdeb_5aa_trampoline_abi_probe:
    move.l  %a0,(%a4)
    move.l  %a1,4(%a4)
    move.l  %a2,8(%a4)
    move.l  %a3,12(%a4)
    moveq   #0,%d0
    rts

m68kdeb_5aa_trampoline_reversible_probe:
    moveq   #0,%d0
    rts
m68kdeb_5aa_trampoline_end:
