/*
 * M1.3b.4b.5z CI-only transfer wrapper.
 *
 * Stages the same a0-a3 takeover ABI used by the isolated final commit path,
 * then JMPs to the already identity-qualified copied transition entry.
 * The target is intentionally non-returning.  This object is qualification
 * support only and is never linked into the production loader.
 */

    .text
    .align 2
    .globl m68kdeb_68030_jump_transition_entry
    .globl _m68kdeb_68030_jump_transition_entry
    .type m68kdeb_68030_jump_transition_entry,@function

m68kdeb_68030_jump_transition_entry:
_m68kdeb_68030_jump_transition_entry:
    move.l  4(%sp),%a5
    move.l  8(%sp),%a0
    tst.l   %a5
    beq.s   .Lbad
    tst.l   %a0
    beq.s   .Lbad

    move.l  8(%a0),%a1
    move.l 12(%a0),%a2
    move.l 20(%a0),%a3
    jmp     (%a5)

.Lbad:
    moveq   #1,%d0
    rts

    .size m68kdeb_68030_jump_transition_entry,.-m68kdeb_68030_jump_transition_entry
