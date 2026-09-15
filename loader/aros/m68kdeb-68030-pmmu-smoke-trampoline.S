/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU stage bisect.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * This diagnostic deliberately returns immediately after TT0, SRP and TC have
 * been loaded.  It isolates PMMU activation from the subsequent high-half
 * alias instruction fetch.  A returned stage of 0x1111 proves that the CPU can
 * enable this exact TT0/SRP/TC configuration and resume through the transparent
 * lower-half mapping; the next qualification step can then re-enable the alias
 * jump knowing that the failure is after TC activation.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030

	/* Resolve every address before translation is enabled. */
	movel	4(%sp),%a0		/* SRP descriptor pair */
	movel	12(%sp),%a4		/* stage word on physical trampoline page */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tt0),%a5

	/* Match the alias probe setup exactly, but do not perform the alias JMP. */
	movel	#0x00000808,%d0
	movec	%d0,%cacr
	pmove	%a5@,%tt0
	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc
	movew	#0x1111,%a4@

	/* Lower-half execution/data/stack must remain usable through TT0 here. */
	pmove	%a1@(4),%tc
	pflusha
	pmove	%a1@(4),%tt0
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
