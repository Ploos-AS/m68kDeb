/*
 * M1.3b.4b.6b-PMMU: supervisor CACR control.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * Supervisor transition, copied-code ABI, stage write, and RTS already pass
 * under FS-UAE.  Add only the 68030 CACR write used by the PMMU path.  No SRP,
 * PFLUSHA, TT0, TC, or address translation operation is performed here.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	12(%sp),%a0		/* stage */
	movel	#0x00000808,%d0
	movec	%d0,%cacr
	movew	#0x1111,%a0@
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
