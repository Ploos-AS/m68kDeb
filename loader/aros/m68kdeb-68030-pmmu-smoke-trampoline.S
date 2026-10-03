/*
 * M1.3b.4b.6b.66-PMMU: restore the known-good post-TC long read.
 *
 * 6b.65 shows that an immediate TC enable/disable sequence fails on the UAE
 * runtime paths. The historical PASS baseline performed one long read through
 * a1 between those PMOVE operations. Restore only that access and its embedded
 * constant. PASS means the post-enable access/pipeline sequence is required
 * for a clean disable; FAIL means another difference from the old baseline
 * remains relevant.
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
	pmove	%a1@,%tc
	movel	%a1@(8),%d1
	pmove	%a1@(4),%tc
	pflusha
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
	.long	0x13579bdf

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
