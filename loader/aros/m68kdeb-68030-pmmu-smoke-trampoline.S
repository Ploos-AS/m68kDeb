/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU identity-walk probe.
 *
 * int entry(const ULONG srp[2], ULONG alias, UWORD *mmusr_out);
 *
 * Diagnostic phase: exercise PTESTR against an address inside the trampoline
 * page itself.  That page is already part of the known-good identity mapping,
 * so this separates generic PTESTR/page-walk behaviour from the logical alias
 * hierarchy.  The alias and scratch arguments remain part of the ABI but are
 * not used in this phase.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030

	/* Load the SRP descriptor and an identity-mapped probe address first. */
	movel	4(%sp),%a0		/* SRP descriptor pair */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1

	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc

	/* Probe the known-good identity mapping, not the logical alias path. */
	ptestr	#5,%a1@,#7

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
