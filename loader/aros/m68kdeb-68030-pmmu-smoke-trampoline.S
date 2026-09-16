/*
 * M1.3b.4b.6b-PMMU: supervisor SRP-load control.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * Supervisor transition and CACR 0x0808 already pass under FS-UAE. Add only
 * loading the supplied 68030 SRP. PMMU translation remains disabled: there is
 * no PFLUSHA, TT0, TC, PTESTR, or alias access in this control.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a1		/* srp */
	movel	12(%sp),%a0		/* stage */
	movel	#0x00000808,%d0
	movec	%d0,%cacr
	pmove	%a1@,%srp
	movew	#0x1111,%a0@
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
