/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU smoke trampoline.
 *
 * int entry(const ULONG srp[2]);
 *
 * Data reads through LOGICAL_ALIAS_PAGE pass, while instruction fetches
 * through the same alias fail even after an explicit instruction-cache
 * invalidation. Keep execution on the known-good physical identity mapping
 * and make one new observation only: ask the MC68030 PMMU to translate the
 * alias explicitly as supervisor-program space (FC=6) with PTEST.
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

	/* The failing jump is a supervisor-program access (FC=6), whereas the
	 * passing ordinary data read is supervisor-data (FC=5). PTEST the same
	 * logical alias explicitly with FC=6 while continuing to fetch code from
	 * the physical identity mapping. Level 7 forces a table walk rather than
	 * relying on an existing ATC entry. */
	movel	#M68KDEB_LOGICAL_ALIAS_PAGE,%a0
	ptestr	#6,%a0@,#7

	/* GNU m68k syntax names the 68030 MMUSR as PSR. It is a 16-bit PMMU
	 * status register, so move it to a word-sized relocation-free scratch
	 * slot before applying the existing invalid/bus/limit mask. */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_mmusr),%a0
	pmove	%psr,%a0@
	moveq	#0,%d0
	movew	%a0@,%d0
	movel	%d0,%d1
	andil	#0x0000c400,%d0
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
m68kdeb_68030_pmmu_smoke_mmusr:
	.word	0x0000

m68kdeb_68030_pmmu_smoke_end:
