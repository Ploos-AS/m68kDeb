/*
 * m68kDeb M1.3b.4b.5a isolated MC68030 cache/MMU instruction probe.
 *
 * This object is compile-only and MUST NOT be linked into the loader or run.
 * It exists to qualify the Bebbo/GNU assembler spelling and encodings needed
 * for the later irreversible 68030 takeover path.
 *
 * The real commit path will only consume these operations after this probe is
 * green and after ordering/state-preservation has been reviewed separately.
 */

    .text
    .align  2
    .globl  m68kdeb_takeover_68030_cache_mmu_instruction_probe
    .type   m68kdeb_takeover_68030_cache_mmu_instruction_probe,@function

m68kdeb_takeover_68030_cache_mmu_instruction_probe:
    /* CACR is supervisor-only; zero disables both MC68030 caches. */
    moveq   #0,%d0
    movec   %d0,%cacr

    /*
     * TC bit 31 is the translation-enable bit.  A zero TC therefore expresses
     * the instruction form required to disable translation in the eventual
     * no-return takeover path.  Keep the source operand in memory because
     * PMOVE uses an effective-address operand.
     */
    lea     .Lzero_tc(%pc),%a0
    pmove   (%a0),%tc

    /* Flush the MC68030/68851-style ATC after changing translation state. */
    pflusha
    rts

    .align  2
.Lzero_tc:
    .long   0

    .size m68kdeb_takeover_68030_cache_mmu_instruction_probe,.-m68kdeb_takeover_68030_cache_mmu_instruction_probe
