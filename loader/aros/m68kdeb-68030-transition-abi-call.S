/* Reversible 5v caller for a copied transition ABI probe. */

    .text
    .align 2
    .globl m68kdeb_68030_call_transition_abi_probe
    .globl _m68kdeb_68030_call_transition_abi_probe
    .type m68kdeb_68030_call_transition_abi_probe,@function

/*
 * C ABI:
 * int call(probe, request, capture)
 *
 * Loads the exact a0-a3 state staged by m68kdeb_takeover_68030_commit(),
 * supplies a4 only as reversible capture storage, and calls the copied probe.
 */
m68kdeb_68030_call_transition_abi_probe:
_m68kdeb_68030_call_transition_abi_probe:
    movem.l %a2-%a4,-(%sp)
    move.l 16(%sp),%a5
    move.l 20(%sp),%a0
    move.l 24(%sp),%a4
    tst.l %a5
    beq.s .Lbad
    tst.l %a0
    beq.s .Lbad
    tst.l %a4
    beq.s .Lbad

    move.l 8(%a0),%a1
    move.l 12(%a0),%a2
    move.l 20(%a0),%a3
    jsr (%a5)
    bra.s .Ldone
.Lbad:
    moveq #1,%d0
.Ldone:
    movem.l (%sp)+,%a2-%a4
    rts

    .size m68kdeb_68030_call_transition_abi_probe,.-m68kdeb_68030_call_transition_abi_probe
