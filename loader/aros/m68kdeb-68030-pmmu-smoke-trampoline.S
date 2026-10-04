/*
 * M1.3b.4b.6b.88-PMMU: reverse the neutral translated instruction pair.
 *
 * 6b.87 passes the full matrix with NOP followed by MOVE.L d2,d2, while 6b.85
 * exposed a generic-smoke-only failure with NOP+NOP. Reverse the 6b.87 pair
 * without changing the two-instruction translated window or d2/CCR semantics.
 * PASS narrows 6b.85 toward the consecutive-NOP opcode/fetch pattern; FAIL
 * points specifically at a NOP occupying the second translated slot.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a0
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1
	pmove	%a0@,%srp
	pflusha
	movel	%d2,%d3
	moveq	#0,%d2
	pmove	%a1@,%tc
	movel	%d2,%d2
	nop
	movel	%d3,%d2
	pmove	%a1@(4),%tc
	pflusha
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	/* E=1, SRE=1, FCL=0, PS=12, TIA=7, TIB=7, TIC=6. */
	.long	0x82c07760
	.long	0x00000000
	.long	0x13579bdf

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
