/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU alias instruction-fetch probe.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *unused);
 *
 * PTESTR hangs for every non-identity alias tested in FS-UAE.  Test the actual
 * capability needed by the loader instead: fetch one JMP through a logical
 * alias that maps to this same physical trampoline page.  No stack or data
 * access occurs while executing from the alias.
 *
 * Match Linux' 68030 MMU-engage sequence by clearing both 68030 caches with
 * CACR=0x0808 before loading SRP/TC.  This isolates stale-cache state from the
 * remaining non-identity translation failure.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030

	/* Resolve every address before translation is enabled. */
	movel	4(%sp),%a0		/* SRP descriptor pair */
	movel	8(%sp),%a2		/* logical alias page base */
	addl	#(m68kdeb_68030_pmmu_alias_entry-m68kdeb_68030_pmmu_smoke_entry),%a2
	lea	%pc@(m68kdeb_68030_pmmu_physical_resume),%a3
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1

	/* Linux arch/m68k/kernel/head.S uses CACR=0x0808 here. */
	movel	#0x00000808,%d0
	movec	%d0,%cacr
	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc

	/* The next instruction fetch after JMP must be translated via the alias. */
	jmp	%a2@

m68kdeb_68030_pmmu_alias_entry:
	/* Executing here proves the alias instruction fetch.  Touch no stack. */
	jmp	%a3@

m68kdeb_68030_pmmu_physical_resume:
	/* Identity mapping is valid here; disable translation before returning. */
	pmove	%a1@(4),%tc
	pflusha
	movel	#0x00000808,%d0
	movec	%d0,%cacr
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

m68kdeb_68030_pmmu_smoke_end:
