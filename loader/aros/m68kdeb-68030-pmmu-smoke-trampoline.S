/*
 * M1.3b.4b.6b.63-PMMU: isolate translated byte read.
 *
 * 6b.62 fails with a single byte write while TC is active. Initialize the
 * same scratch byte before enabling translation, then perform only a byte read
 * through the same address register while TC is active. PASS isolates the
 * regression to translated writes; FAIL shows ordinary translated data reads
 * are sufficient too.
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
	moveb	#0x5a,%a0@
	pmove	%a1@,%tc
	moveq	#0,%d1
	moveb	%a0@,%d1
	pmove	%a1@(4),%tc
	pflusha
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
