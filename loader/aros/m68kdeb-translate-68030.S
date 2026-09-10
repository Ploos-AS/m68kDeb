/*
 * Reversible MC68030 translation evidence probe.
 *
 * PTEST inspects translation state without changing TC/CRP/SRP/TT registers,
 * the ATC, or descriptor used/modified bits.  The address register result is
 * the physical address of the last translation-table descriptor accessed; it
 * is NOT by itself the translated physical address of the tested byte.
 *
 * On the MC68030 the PTEST status is read through the 16-bit PMMU PSR
 * register (the MMUSR naming/form belongs to later integrated MMUs such as
 * the 68040).  This object is isolated and must not be confused with the
 * irreversible takeover object.
 */

    .text
    .align  2
    .globl  m68kdeb_translate_68030_ptest
    .globl  _m68kdeb_translate_68030_ptest
    .type   m68kdeb_translate_68030_ptest,@function
    .type   _m68kdeb_translate_68030_ptest,@function

/*
 * int m68kdeb_translate_68030_ptest(
 *     unsigned long logical,
 *     unsigned short *psr_out,
 *     unsigned long *last_descriptor_phys_out);
 *
 * Preconditions:
 *   - MC68030 with PMMU (not 68EC030)
 *   - supervisor state
 *   - output pointers valid
 *
 * Function code #5 selects supervisor data space.  Level #7 requests a
 * bottom-of-walk search, stopping earlier on invalid/limit/bus-error state.
 *
 * The AmigaOS GCC toolchain may reference C ABI symbols with a leading
 * underscore.  Export both spellings at the same entry point so this isolated
 * assembly probe links correctly without changing its runtime semantics.
 *
 * A2 and A3 are callee-saved in the Amiga GCC ABI. Preserve them explicitly;
 * failure to do so corrupts caller state after the supervisor PTEST returns.
 */
m68kdeb_translate_68030_ptest:
_m68kdeb_translate_68030_ptest:
    movem.l %a2-%a3,-(%sp)

    move.l 12(%sp),%a0
    move.l 16(%sp),%a2
    move.l 20(%sp),%a3

    tst.l   %a2
    beq.s   .Lbad
    tst.l   %a3
    beq.s   .Lbad

    ptestw  #5,(%a0),#7,%a1
    pmove   %psr,(%a2)
    move.l  %a1,(%a3)

    movem.l (%sp)+,%a2-%a3
    moveq   #0,%d0
    rts

.Lbad:
    movem.l (%sp)+,%a2-%a3
    moveq   #1,%d0
    rts

    .size m68kdeb_translate_68030_ptest,.-m68kdeb_translate_68030_ptest
    .size _m68kdeb_translate_68030_ptest,.-_m68kdeb_translate_68030_ptest
