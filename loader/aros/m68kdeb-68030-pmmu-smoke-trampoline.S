/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 trampoline control.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * This control deliberately performs no PMMU operation. It only clears the
 * 68030 caches, records stage 0x1111, and returns. A successful run proves
 * that supervisor entry, trampoline execution, stage write, and RTS are sound
 * before SRP/PFLUSHA are reintroduced independently.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030

	movel	12(%sp),%a4		/* stage word */

	movel	#0x00000808,%d0
	movec	%d0,%cacr
	movew	#0x1111,%a4@

	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
