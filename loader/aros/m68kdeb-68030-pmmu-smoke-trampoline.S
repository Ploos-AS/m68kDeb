/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU alias probe trampoline.
 *
 * int entry(const ULONG srp[2], ULONG alias, UWORD *mmusr_out);
 *
 * Do not dereference the translated alias yet.  PTESTR walks the configured
 * PMMU hierarchy without turning this diagnostic into an ordinary data access.
 * On the MC68030 GNU as exposes MMUSR as the 16-bit PMMU PSR register.  Store
 * that PSR value to scratch inside the identity-mapped trampoline page, then
 * disable TC before any stack access / return.
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
	movel	12(%sp),%a3		/* identity-mapped 16-bit PSR/MMUSR scratch */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1

	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc

	/* Probe supervisor-data translation only; no ordinary alias dereference. */
	ptestr	#5,%a2@,#7
	pmove	%psr,%a3@

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
