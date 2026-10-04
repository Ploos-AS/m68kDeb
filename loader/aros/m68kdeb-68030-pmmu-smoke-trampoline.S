/*
 * M1.3b.4b.6b.80-PMMU: isolate CCR change from GPR change before TC.
 *
 * 6b.78 fails with two-byte moveq #0,d2 immediately before PMOVE TC, while
 * 6b.79 passes with a two-byte NOP at the identical location. Replace NOP
 * with cmp.l d2,d2: this sets Z and clears N/V/C like moveq #0 without
 * changing d2. PASS isolates the failure to GPR mutation rather than CCR.
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
	cmpl	%d2,%d2
	pmove	%a1@,%tc
	nop
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
