/*
 * M1.3b.4b.6b.87-PMMU: distinguish two translated instructions from NOP pair.
 *
 * 6b.85 fails only in the generic smoke runtime with two translated NOPs,
 * while 6b.86 passes when the second NOP is skipped. Execute two translated
 * instructions again, but make the second a register self-move that preserves
 * d2 and CCR. FAIL points to instruction-count/duration; PASS points to the
 * consecutive-NOP/fetch pattern.
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
	nop
	movel	%d2,%d2
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
