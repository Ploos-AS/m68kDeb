/*
 * M1.3b.4b.6b.54-PMMU: isolate absolute-long data addressing after TC.
 *
 * 6b.53 proves PC-relative effective-address calculation plus register-indirect
 * JMP is safe after PMOVE TC. Use an absolute-long memory read while
 * translation is active, but retain the known-good register-indirect control
 * transfer. The qualifier patches the sentinel to the embedded constant.
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
	movel	0xdeadbeef.l,%d1
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
m68kdeb_68030_pmmu_smoke_value:
	.long	0x13579bdf

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
