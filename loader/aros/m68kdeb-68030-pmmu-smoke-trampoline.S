/*
 * M1.3b.4b.6b.90-PMMU: Linux-like post-TC jump with immediate SRP switch.
 *
 * 6b.89 proves PMOVE TC followed by a translated long jump works across the
 * full qualifier matrix. Linux m68k head.S then switches from the bootstrap
 * root pointer to its final SRP after the post-enable jump. Add that exact
 * class of PMMU operation here: jump under translation, PMOVE a second valid
 * root pointer into SRP, flush ATC, then restore the qualifier state.
 *
 * The second SRP image intentionally names the same known-good root as the
 * bootstrap SRP. This isolates the act/timing of the post-TC SRP switch from
 * any page-table topology change.
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
	jmp	1f
1:
	pmove	%a0@,%srp
	pflusha
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
