/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU alias probe trampoline.
 *
 * int entry(const ULONG srp[2], ULONG alias, UWORD *mmusr_out);
 *
 * Diagnostic phase: isolate the page-table walk itself.  PTESTR walks the
 * configured PMMU hierarchy without an ordinary data access.  Do not read
 * PSR/MMUSR in this phase; if this sequence returns, PSR capture is the next
 * variable to test independently.
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
	movel	12(%sp),%a3		/* reserved scratch argument; unused in this phase */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1

	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc

	/* Isolate supervisor-data PTESTR; no alias dereference and no PSR readback. */
	ptestr	#5,%a2@,#7

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
