/*
 * M1.3b.4b.6a MC68030 Linux-transfer trampoline.
 *
 * CI/static qualification first. 5z already qualified the irreversible
 * CACR=0, TC=0, PFLUSHA sequence under FS-UAE. This variant replaces the
 * 5z local containment loop with the one intended final transfer: JMP (a1).
 *
 * Entry register contract (qualified by 5v/5x/5y):
 *   a0 = takeover request
 *   a1 = Linux entry
 *   a2 = bootinfo
 *   a3 = initramfs
 *
 * The blob is self-contained and relocation-free. No stack access, external
 * reference or return is permitted after entry.
 */

    .text
    .align  2
    .globl  m68kdeb_takeover_68030_linux_start
    .globl  m68kdeb_takeover_68030_linux_entry
    .globl  m68kdeb_takeover_68030_linux_mmu_off
    .globl  m68kdeb_takeover_68030_linux_post_mmu
    .globl  m68kdeb_takeover_68030_linux_jump
    .globl  m68kdeb_takeover_68030_linux_tc_zero
    .globl  m68kdeb_takeover_68030_linux_end
    .type   m68kdeb_takeover_68030_linux_entry,@function

m68kdeb_takeover_68030_linux_start:
m68kdeb_takeover_68030_linux_entry:
    moveq   #0,%d0
    movec   %d0,%cacr

    lea     m68kdeb_takeover_68030_linux_tc_zero(%pc),%a0
m68kdeb_takeover_68030_linux_mmu_off:
    pmove   (%a0),%tc

m68kdeb_takeover_68030_linux_post_mmu:
    pflusha

m68kdeb_takeover_68030_linux_jump:
    jmp     (%a1)

    .align  2
m68kdeb_takeover_68030_linux_tc_zero:
    .long   0
m68kdeb_takeover_68030_linux_end:

    .size m68kdeb_takeover_68030_linux_entry,.-m68kdeb_takeover_68030_linux_entry
