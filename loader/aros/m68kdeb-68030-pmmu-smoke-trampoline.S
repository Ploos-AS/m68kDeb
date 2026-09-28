/*
 * M1.3b.4b.6b.48-PMMU: isolate save/restore frame around TC.
 *
 * 6b.44 fails with a direct d2 write, while 6b.46/47 pass after adding a
 * d2->d3 save and post-TC restore. Preserve that save/restore frame but do
 * not modify d2 while translation is active. This isolates whether the
 * passing behaviour comes from the frame itself rather than the TC-time value.
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
	pmove	%a1@,%tc
	movel	%a1@(8),%d1
	pmove	%a1@(4),%tc
	pflusha
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
	.long	0x13579bdf

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
