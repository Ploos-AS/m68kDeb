/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU smoke trampoline.
 *
 * int entry(const ULONG srp[2]);
 *
 * The descriptor setup is the known-PASS dual mapping. The previous alias
 * instruction-fetch experiment failed while a data read through the same
 * alias passed. This experiment changes one thing relative to the failing
 * fetch case: explicitly invalidate the MC68030 instruction cache after TC
 * is enabled and immediately before jumping through LOGICAL_ALIAS_PAGE.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

	.equ	M68KDEB_LOGICAL_ALIAS_PAGE, 0x005db000
	.equ	M68KDEB_CACR_CLEAR_I, 0x00000008

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a0
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1
	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc

	/* Preserve the current CACR policy while issuing the MC68030 clear-I
	 * command (CACR bit 3). This is the sole semantic delta from the prior
	 * failing alias instruction-fetch experiment. */
	movec	%cacr,%d1
	oril	#M68KDEB_CACR_CLEAR_I,%d1
	movec	%d1,%cacr

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
