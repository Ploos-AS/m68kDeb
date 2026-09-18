/*
 * M1.3b.4b.6b-PMMU: translated stage-write control.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * The pure TC enable/disable baseline passes, while adding TT0 hangs. Leave
 * TT0 out and test one translated data access while TC is active. The stage
 * word lives at the end of the same identity-mapped page as this trampoline,
 * so both instruction fetch and the stage write use the known hierarchy.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a1		/* srp */
	movel	12(%sp),%a0		/* stage */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a2
	pmove	%a1@,%srp
	pflusha
	pmove	%a2@,%tc
	movew	#0x1111,%a0@		/* translated identity-mapped write */
	pmove	%a2@(4),%tc
	pflusha
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
