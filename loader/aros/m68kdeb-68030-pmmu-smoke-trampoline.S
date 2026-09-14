/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU alias-walk bisection.
 *
 * int entry(const ULONG srp[2], ULONG alias, UWORD *mmusr_out);
 *
 * Diagnostic phase: exercise PTESTR against the logical alias at level 0.
 * The level-1 alias probe still hung after descriptor-path preflight and table
 * alignment fixes, so this isolates the earliest possible alias-specific
 * translation-search step.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030

	/* Load all caller data before enabling translation. */
	movel	4(%sp),%a0		/* SRP descriptor pair */
	movel	8(%sp),%a2		/* logical alias */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1

	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc

	/* Alias walk bisection: level 0 only. */
	ptestr	#5,%a2@,#0

	/* Return to the known-good non-translated environment. */
	pmove	%a1@(4),%tc
	pflusha
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

m68kdeb_68030_pmmu_smoke_end:
