/*
 * M1.3b.4b.6b.82-PMMU: isolate which GPR value matters at TC enable.
 *
 * 6b.81 passes when d2 is transiently cleared and restored before PMOVE TC,
 * while 6b.78 fails when d2 remains zero. Keep d2 untouched and force d3=0
 * immediately before TC. This tests whether the failure follows any changed
 * GPR value at enable or is specific to d2.
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
	moveq	#0,%d3
	pmove	%a1@,%tc
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
