/*
 * M1.3b.4b.6b.97-PMMU: compare external and local data under active TC.
 *
 * 6b.95 fails when an ordinary post-TC longword read dereferences the original
 * external a0 pointer. 6b.96 passes when the same pointer value is only copied.
 * Snapshot the first external longword into local PC-relative storage before
 * enabling TC, then read that local copy after the translated jump.
 *
 * PASS shows ordinary post-TC data reads are viable and isolates the failure
 * to translation/reachability of the external a0 region rather than MOVEL or
 * post-TC data access in general.
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
	movel	%a0@,%a1@(20)
	movel	%d2,%d3
	moveq	#0,%d2
	pmove	%a1@,%tc
	jmp	1f
1:
	pmove	%srp,%a1@(12)
	pmove	%a1@(12),%srp
	movel	%a1@(20),%d2
	movel	%d3,%d2
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
	.long	0x00000000
	.long	0x00000000
	.long	0x00000000

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
