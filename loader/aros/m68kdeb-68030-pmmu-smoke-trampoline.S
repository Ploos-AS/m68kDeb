/*
 * M1.3b.4b.6b.64-PMMU: control with no translated data access.
 *
 * 6b.62 and 6b.63 fail with a single translated byte write or read. Keep the
 * same scratch setup and TC enable/disable lifetime, but perform no explicit
 * data access while translation is active. PASS proves the minimal failures
 * require the translated data access itself; FAIL moves the boundary back to
 * the surrounding TC/register sequence.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a0
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1
	pmove	%a0@,%srp
	pflusha
	movel	%d2,%d3
	lea	%pc@(m68kdeb_68030_pmmu_smoke_scratch),%a0
	moveb	#0x5a,%a0@
	pmove	%a1@,%tc
	nop
	pmove	%a1@(4),%tc
	pflusha
	moveq	#0,%d1
	moveb	%a0@,%d1
	movel	%d3,%d2
	moveq	#0,%d0
	cmpb	#0x5a,%d1
	beq	1f
	moveq	#1,%d0
1:
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

	.balign 4
m68kdeb_68030_pmmu_smoke_scratch:
	.long	0x00000000

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
