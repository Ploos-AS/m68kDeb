/*
 * M1.3b.4b.6b-PMMU: supervisor TC-enable control.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * Supervisor transition, CACR 0x0808, valid SRP load, PFLUSHA, and TT0 load
 * already pass under FS-UAE. Add only the known 68030 TC value. No PTESTR or
 * alias access is performed after translation is enabled.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a1		/* srp */
	movel	12(%sp),%a0		/* stage */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tt0),%a2
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a3
	movel	#0x00000808,%d0
	movec	%d0,%cacr
	pmove	%a1@,%srp
	pflusha
	pmove	%a2@,%tt0
	pmove	%a3@,%tc
	movew	#0x1111,%a0@
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tt0:
	/* Existing low-2-GiB transparent-translation descriptor. */
	.long	0x007f8107

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	/* E=1, SRE=1, PS=12, TIA=7, TIB=7, TIC=6. */
	.long	0x82c07760

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
