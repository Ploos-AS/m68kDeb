/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU level-0 control probe.
 *
 * int entry(const ULONG srp[2], ULONG alias, UWORD *mmusr_out);
 *
 * Diagnostic phase: exercise PTESTR at level 0 against an address inside the
 * trampoline page itself.  The same level against the logical alias hung, so
 * this separates level-0 PTESTR semantics from alias-specific translation.
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

	/* Level-0 control: probe the known-good identity mapping. */
	ptestr	#5,%a1@,#0

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
