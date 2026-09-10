/* Reversible MC68030 translation-control observation. */

    .text
    .align  2
    .globl  m68kdeb_68030_read_tc
    .globl  _m68kdeb_68030_read_tc
    .type   m68kdeb_68030_read_tc,@function
    .type   _m68kdeb_68030_read_tc,@function

/*
 * int m68kdeb_68030_read_tc(unsigned long *tc_out);
 *
 * Preconditions:
 *   - MC68030 with PMMU
 *   - supervisor state
 *   - tc_out points to writable memory
 *
 * This is observation only: PMOVE transfers TC to memory. It never writes TC
 * or any other MMU control register and never flushes the ATC.
 */
m68kdeb_68030_read_tc:
_m68kdeb_68030_read_tc:
    move.l  4(%sp),%a0
    tst.l   %a0
    beq.s   .Lbad_tc

    pmove   %tc,(%a0)
    moveq   #0,%d0
    rts

.Lbad_tc:
    moveq   #1,%d0
    rts

    .size m68kdeb_68030_read_tc,.-m68kdeb_68030_read_tc
    .size _m68kdeb_68030_read_tc,.-_m68kdeb_68030_read_tc
