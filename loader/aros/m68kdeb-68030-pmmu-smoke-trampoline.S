/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU smoke trampoline.
 *
 * int entry(const ULONG srp[2]);
 *
 * Data reads and an explicit supervisor-program PTEST through
 * LOGICAL_ALIAS_PAGE pass, while actual instruction fetches through the
 * alias fail even after an instruction-cache invalidation. This experiment
 * changes one thing relative to the failing fetch case: disable the MC68030
 * instruction cache (CACR.EI=bit 0) before enabling translation and jumping
 * through the logical alias. Restore CACR only after translation is disabled.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

	.equ	M68KDEB_LOGICAL_ALIAS_PAGE, 0x005db000
	.equ	M68KDEB_CACR_ENABLE_I, 0x00000001
	.equ	M68KDEB_CACR_CLEAR_I,  0x00000008

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a0
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1

	/* Save the caller's cache policy in a data register. Disable EI and clear
	 * any valid I-cache entries before the PMMU transition. No stack accesses
	 * are needed after TC is enabled. */
	movec	%cacr,%d2
	movel	%d2,%d1
	andil	#0xfffffffe,%d1
	oril	#M68KDEB_CACR_CLEAR_I,%d1
	movec	%d1,%cacr

	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc

	movel	#M68KDEB_LOGICAL_ALIAS_PAGE+(m68kdeb_68030_pmmu_smoke_alias-m68kdeb_68030_pmmu_smoke_entry),%a0
	jmp	%a0@

m68kdeb_68030_pmmu_smoke_alias:
	pmove	%a1@(4),%tc
	pflusha
	movec	%d2,%cacr
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

m68kdeb_68030_pmmu_smoke_end:
