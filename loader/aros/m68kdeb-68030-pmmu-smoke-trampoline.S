/*
 * M1.3b.4b.6b.61-PMMU: compare identical data write before and during TC.
 *
 * 6b.60 shows that an ordinary same-page data write fails while translation
 * is active. Perform the identical address-register write once before PMMU
 * enable and once after PMOVE TC. The first write controls for the target,
 * instruction and memory itself; failure after the second isolates translated
 * write handling.
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
	movel	#0x2468ace0,%a0@	/* control write with TC disabled */
	pmove	%a1@,%tc
	movel	#0x13579bdf,%a0@	/* identical write with TC enabled */
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
