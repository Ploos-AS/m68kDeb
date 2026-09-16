/*
 * M1.3b.4b.6b-PMMU: bare copied-code/ABI trampoline control.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * Deliberately performs no PMMU, cache, memory, or argument access.  This is
 * the smallest useful bisect: execute the copied blob, return zero in D0, and
 * RTS to the C caller.  If this does not return, the failure is below the PMMU
 * and stage-write layers.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
