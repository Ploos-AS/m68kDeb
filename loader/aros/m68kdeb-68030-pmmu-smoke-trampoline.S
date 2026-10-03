/*
 * M1.3b.4b.6b.62-PMMU: isolate translated byte write.
 *
 * 6b.61 proves the same address-register long write succeeds with TC disabled
 * and fails with TC enabled. Reduce the active-translation operation to one
 * byte write. FAIL means the regression applies to translated writes in
 * general; PASS narrows it to wider write cycles.
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
	lea	%pc@(m68kdeb_68030_pmmu_smoke_scratch),%a0
	clrb	%a0@
	pmove	%a1@,%tc
	moveb	#0x5a,%a0@
	pmove	%a1@(4),%tc
	pflusha
	moveq	#0,%d1
	moveb	%a0@,%d1
	movel	%d3,%d2
	moveq	#0,%d0
	cmpb	#0x5a,%d1
	beq	1f
	moveq	#1,%d0
1:
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

	.balign 4
m68kdeb_68030_pmmu_smoke_scratch:
	.long	0x00000000

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
