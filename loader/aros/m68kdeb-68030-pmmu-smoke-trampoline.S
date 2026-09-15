/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU stage bisect.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * This diagnostic isolates SRP loading from TT0 and TC. It loads the cache
 * state and SRP, flushes the ATC, records stage 0x1111, and returns while
 * translation remains disabled. If this returns, the previous hang is caused
 * by TT0 setup rather than SRP/PFLUSHA.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030

	movel	4(%sp),%a0		/* SRP descriptor pair */
	movel	12(%sp),%a4		/* stage word */

	movel	#0x00000808,%d0
	movec	%d0,%cacr
	/* TT0 intentionally untouched in this bisect. */
	pmove	%a0@,%srp
	pflusha
	/* TC intentionally remains disabled. */
	movew	#0x1111,%a4@

	/* Translation was never enabled; return directly. */
	pflusha
	movel	#0x00000808,%d0
	movec	%d0,%cacr
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

m68kdeb_68030_pmmu_smoke_end:
