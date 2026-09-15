/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU stage bisect.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * This diagnostic loads the cache state, TT0 and SRP used by the alias probe,
 * but deliberately leaves TC disabled.  Returning with stage 0x1111 proves
 * that TT0/SRP setup itself is safe; TC activation is then the remaining
 * boundary to isolate.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030

	movel	4(%sp),%a0		/* SRP descriptor pair */
	movel	12(%sp),%a4		/* stage word */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tt0),%a5

	movel	#0x00000808,%d0
	movec	%d0,%cacr
	pmove	%a5@,%tt0
	pmove	%a0@,%srp
	pflusha
	/* TC intentionally remains disabled in this bisect. */
	movew	#0x1111,%a4@

	/* Restore TT0 before returning; TC was never enabled. */
	pmove	%a1@(4),%tt0
	pflusha
	movel	#0x00000808,%d0
	movec	%d0,%cacr
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

	/* Lower 2 GiB transparent for all function codes/read-write accesses. */
m68kdeb_68030_pmmu_smoke_tt0:
	.long	0x007f8107

m68kdeb_68030_pmmu_smoke_end:
