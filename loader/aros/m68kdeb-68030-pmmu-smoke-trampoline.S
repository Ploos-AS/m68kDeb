/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU smoke trampoline.
 *
 * int entry(const ULONG srp[2]);
 *
 * Data reads and explicit supervisor-program PTEST translation through
 * LOGICAL_ALIAS_PAGE pass, while actual instruction fetches through the
 * alias fail. This experiment changes one thing relative to the failing
 * alias-fetch case: preload the alias translation into the ATC explicitly
 * as supervisor-program space (FC=6) with PLOADR before jumping through it.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

	.equ	M68KDEB_LOGICAL_ALIAS_PAGE, 0x005db000

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a0
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1

	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc

	/* Populate the ATC for the same supervisor-program access that the
	 * following instruction fetch will perform. If the jump starts working,
	 * the failure is in fetch-side table walking/ATC population rather than
	 * execution through the translated alias itself. */
	movel	#M68KDEB_LOGICAL_ALIAS_PAGE,%a0
	ploadr	#6,%a0@

	movel	#M68KDEB_LOGICAL_ALIAS_PAGE+(m68kdeb_68030_pmmu_smoke_alias-m68kdeb_68030_pmmu_smoke_entry),%a0
	jmp	%a0@

m68kdeb_68030_pmmu_smoke_alias:
	pmove	%a1@(4),%tc
	pflusha
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

m68kdeb_68030_pmmu_smoke_end:
