/*
 * M1.3b.4b.6b.73-PMMU: establish non-zero GPR only after TC enable.
 *
 * 6b.72 fails in the restored historical PASS envelope when d2=1 is set
 * immediately before PMOVE TC. Move that sole state change to immediately
 * after PMOVE TC, retaining the same translated same-page data read. FAIL
 * means unrelated non-zero GPR state during active translation is sufficient;
 * PASS isolates the trigger to state present at the TC-enable transition.
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
	moveq	#1,%d2
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
