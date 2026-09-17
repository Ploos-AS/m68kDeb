/*
 * M1.3b.4b.6b-PMMU: supervisor TC enable -> disable control.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * Use the real identity-mapped SRP hierarchy prepared by the qualifier and
 * reproduce the previously known-good TC control as closely as possible:
 * SRP -> PFLUSHA -> TC enable -> immediate TC disable -> PFLUSHA -> return.
 * Do not load TT0, touch stage memory, perform PTESTR, or access an alias while
 * translation is active. This isolates TC activation from the TT0 path.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a0		/* srp */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1
	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc
	pmove	%a1@(4),%tc
	pflusha
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	/* E=1, SRE=1, PS=12, TIA=7, TIB=7, TIC=6; followed by TC=0. */
	.long	0x82c07760
	.long	0x00000000

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
