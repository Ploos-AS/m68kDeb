/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU control baseline.
 *
 * int entry(const ULONG srp[2], ULONG alias, UWORD *mmusr_out);
 *
 * Diagnostic phase: keep the current SRP/page-table setup, but perform no
 * PTESTR, alias access, or PSR/MMUSR readback.  This re-validates the known-good
 * PMMU control path with the current hierarchy: TC enable -> TC disable ->
 * return.  The alias and scratch arguments remain part of the ABI but are not
 * used in this phase.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030

	/* Load only the SRP descriptor before enabling translation. */
	movel	4(%sp),%a0		/* SRP descriptor pair */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1

	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc

	/* Pure control baseline: no PMMU walk and no translated access. */
	pmove	%a1@(4),%tc
	pflusha
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

m68kdeb_68030_pmmu_smoke_end:
