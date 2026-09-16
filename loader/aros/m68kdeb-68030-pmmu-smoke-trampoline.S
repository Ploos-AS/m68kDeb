/*
 * M1.3b.4b.6b-PMMU: copied-code ABI stage-write control.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * Bare RTS already passes.  Add only the third C argument load and a single
 * word write.  No supervisor switch, cache-register access, or PMMU operation
 * is performed here.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	12(%sp),%a0		/* stage */
	movew	#0x1111,%a0@
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
