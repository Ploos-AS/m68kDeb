/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU smoke trampoline.
 *
 * int entry(const ULONG srp[2], ULONG phase);
 *
 * phase 1: enable translation over the identity-mapped trampoline page,
 *          then disable it and return.
 * phase 2: phase 1 plus PLOADR of the supervisor-program alias.
 * phase 3: phase 2 plus the actual instruction fetch through the alias.
 *
 * Splitting the irreversible boundary lets the guest persist a marker after
 * each successful return. If an emulator wedges during a PMMU operation, the
 * host can therefore identify the exact last completed boundary instead of
 * reporting only a generic "armed but did not return" result.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

	.equ	M68KDEB_LOGICAL_ALIAS_PAGE, 0x005db000

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a0
	movel	8(%sp),%d1
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1

	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc

	/* Phase 1 proves that instruction fetch can continue on the physical ==
	 * logical trampoline page while translation is enabled. */
	cmpil	#1,%d1
	beq.s	m68kdeb_68030_pmmu_smoke_disable

	/* Phase 2 explicitly populates the ATC for the supervisor-program alias.
	 * PLOADR uses FC=6, matching the following supervisor instruction fetch. */
	movel	#M68KDEB_LOGICAL_ALIAS_PAGE,%a0
	ploadr	#6,%a0@
	cmpil	#2,%d1
	beq.s	m68kdeb_68030_pmmu_smoke_disable

	/* Phase 3 is the actual translated instruction-fetch boundary. */
	movel	#M68KDEB_LOGICAL_ALIAS_PAGE+(m68kdeb_68030_pmmu_smoke_alias-m68kdeb_68030_pmmu_smoke_entry),%a0
	jmp	%a0@

m68kdeb_68030_pmmu_smoke_alias:
m68kdeb_68030_pmmu_smoke_disable:
	pmove	%a1@(4),%tc
	pflusha
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

m68kdeb_68030_pmmu_smoke_end:
