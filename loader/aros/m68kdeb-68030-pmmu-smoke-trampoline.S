/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU smoke trampoline.
 *
 * int entry(const ULONG srp[2]);
 *
 * The descriptor setup is the known-PASS dual mapping. This experiment adds
 * one semantic delta only: after TC is enabled, perform a data read through
 * LOGICAL_ALIAS_PAGE while instruction fetch remains on the physical identity
 * mapping. The aliased longword must match the physical entry longword.
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

	/* Keep executing through the physical identity map, but translate one
	 * data access through the low logical alias. */
	movel	#M68KDEB_LOGICAL_ALIAS_PAGE,%a0
	movel	%a0@,%d0
	lea	%pc@(m68kdeb_68030_pmmu_smoke_entry),%a0
	cmpl	%a0@,%d0
	bne	m68kdeb_68030_pmmu_smoke_fail

	pmove	%a1@(4),%tc
	pflusha
	moveq	#0,%d0
	rts

m68kdeb_68030_pmmu_smoke_fail:
	pmove	%a1@(4),%tc
	pflusha
	moveq	#1,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

m68kdeb_68030_pmmu_smoke_end:
