/*
 * M1.3b.4b.6b-PMMU: TT0 + TC enable -> disable control.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * The real identity-mapped SRP hierarchy and the pure
 * SRP -> PFLUSHA -> TC enable -> TC disable -> PFLUSHA -> return sequence
 * pass under FS-UAE. Add exactly one previously removed element: load the
 * established TT0 descriptor before enabling TC. Keep the active-translation
 * window otherwise identical: no stage write, PTESTR, or alias access.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a0		/* srp */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tt0),%a2
	pmove	%a0@,%srp
	pflusha
	pmove	%a2@,%tt0
	pmove	%a1@,%tc
	pmove	%a1@(4),%tc
	pflusha
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tt0:
	/* Existing low-2-GiB transparent-translation descriptor. */
	.long	0x007f8107

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	/* E=1, SRE=1, PS=12, TIA=7, TIB=7, TIC=6; followed by TC=0. */
	.long	0x82c07760
	.long	0x00000000

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
