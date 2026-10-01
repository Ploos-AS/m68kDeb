/*
 * M1.3b.4b.6b.60-PMMU: compare ordinary translated data write.
 *
 * 6b.59 fails when the only translated data operation is a predecrement stack
 * write. Write the same value to an ordinary same-page data location through
 * a precomputed address register while TC is active. PASS isolates the failure
 * to SP/predecrement stack access; FAIL means translated data writes generally
 * are affected.
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
	pmove	%a1@,%tc
	movel	#0x13579bdf,%a0@
	pmove	%a1@(4),%tc
	pflusha
	movel	%a0@,%d1
	movel	%d3,%d2
	moveq	#0,%d0
	cmpl	#0x13579bdf,%d1
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
