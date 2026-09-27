/*
 * M1.3b.4b.6b.44-PMMU: isolate register-write history before TC.
 *
 * 6b.43 failed even though d2 was restored to zero before PMOVE TC. Test the
 * smallest possible state change: force d2 to zero once before TC, with no
 * preceding non-zero value. This separates the historical 1->0 transition
 * from simply executing an instruction that writes d2 before PMMU enable.
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
	clrl	%d2
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
