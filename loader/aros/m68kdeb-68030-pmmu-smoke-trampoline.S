/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU alias probe trampoline.
 *
 * int entry(const ULONG srp[2], ULONG alias, ULONG *mmusr_out);
 *
 * Do not dereference the translated alias yet.  PTESTR walks the configured
 * PMMU hierarchy without turning this diagnostic into an ordinary data access.
 * MMUSR is written to scratch storage inside the identity-mapped trampoline
 * page, then TC is disabled before stack access / return.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030

	/* Load every caller argument before enabling translation. */
	movel	4(%sp),%a0		/* SRP descriptor pair */
	movel	8(%sp),%a2		/* logical alias */
	movel	12(%sp),%a3		/* identity-mapped MMUSR scratch */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1

	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc

	/* Probe supervisor-data translation only; no ordinary alias dereference. */
	ptestr	#1,%a2@,#7
	pmove	%mmusr,%a3@

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
