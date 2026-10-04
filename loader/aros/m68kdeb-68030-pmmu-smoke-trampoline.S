/*
 * M1.3b.4b.6b.83-PMMU: isolate d2 value at the TC-enable boundary.
 *
 * 6b.81 passes when d2 is restored before PMOVE TC, while 6b.78 fails when
 * d2 remains zero across enable. Save d2, force zero for PMOVE TC, then restore
 * it immediately after enable before the control NOP. FAIL means the d2 value
 * at TC enable itself is sufficient; PASS means d2 must remain changed while
 * translated execution continues.
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
	movel	%d3,%d2
	nop
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
