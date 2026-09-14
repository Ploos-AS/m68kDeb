/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU smoke trampoline.
 *
 * int entry(const ULONG crp[2], ULONG phase);
 *
 * phase 1: enable translation over the identity-mapped transition window,
 *          then disable it and return.
 * phase 2: phase 1 plus PLOADR of the supervisor-program alias.
 * phase 3: phase 2 plus the actual instruction fetch through the alias.
 *
 * Keep the minimal qualification path on CRP with TC.SRE=0. This removes
 * supervisor-root selection from the enable/disable boundary while keeping
 * the exact same translation tree. Once the base transition is proven, SRP
 * selection can be qualified independently instead of being coupled to it.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

	.equ	M68KDEB_LOGICAL_ALIAS_PAGE, 0x01000000

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a0
	movel	8(%sp),%d1
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1

	/* TC.SRE is clear below, so both supervisor and user translations use
	 * CRP. Load CRP before enabling translation. */
	pmove	%a0@,%crp
	pflusha
	pmove	%a1@,%tc

	cmpil	#1,%d1
	beq.s	m68kdeb_68030_pmmu_smoke_disable

	movel	#M68KDEB_LOGICAL_ALIAS_PAGE,%a0
	ploadr	#6,%a0@
	cmpil	#2,%d1
	beq.s	m68kdeb_68030_pmmu_smoke_disable

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
	/* E=1, SRE=0, FCL=0, PS=12, IS=0, TIA/TIB/TIC=7/7/6.
	 * 12 + 7 + 7 + 6 = 32, satisfying the MC68030 TC consistency rule. */
	.long	0x80c07760
	.long	0x00000000

m68kdeb_68030_pmmu_smoke_end:
