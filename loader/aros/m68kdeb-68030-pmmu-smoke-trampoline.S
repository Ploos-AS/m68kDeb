/*
 * M1.3b.4b.6b.86-PMMU: separate translated instruction count from layout.
 *
 * 6b.85 passes on FS-UAE, FS-UAE 3.2.35, and Amiberry but fails in the
 * generic smoke runtime with two translated NOPs. Preserve the two-NOP code
 * footprint while branching over the second NOP. This keeps the later
 * instructions displaced while executing only one NOP before restoring d2.
 * PASS points to executed-instruction count; FAIL points to layout/fetch.
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
	moveq	#0,%d2
	pmove	%a1@,%tc
	nop
	bra.s	1f
	nop
1:
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

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
