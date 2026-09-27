/*
 * M1.3b.4b.6b.43-PMMU: isolate register value at TC enable.
 *
 * The prior bisect shows d2=0 before TC passes while d2=1 fails, and moving
 * d2=1 after TC also fails. This variant writes a non-zero value and restores
 * zero before PMOVE TC. PASS means the critical state is the register value
 * observed at TC enable, not merely executing a register-write instruction.
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
	movel	#0x00000001,%d2
	clrl	%d2
	pmove	%a1@,%tc
	movel	%a1@(8),%d1
	pmove	%a1@(4),%tc
	pflusha
	moveq	#0,%d0
	cmpl	#0x13579bdf,%d1
	beq	1f
	moveq	#1,%d0
1:
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000
	.long	0x13579bdf

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
