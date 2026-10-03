/*
 * M1.3b.4b.6b.67-PMMU: restore exact historical PASS trampoline.
 *
 * 6b.65 and 6b.66 fail on the FS-UAE paths. Re-establish the exact instruction
 * sequence that previously passed before doing any further differential
 * testing. This separates trampoline semantics from changes elsewhere in the
 * current qualification environment.
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
	/* E=1, SRE=1, FCL=0, PS=12, TIA=7, TIB=7, TIC=6. */
	.long	0x82c07760
	.long	0x00000000
	.long	0x13579bdf

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
