/*
 * M1.3b.4b.6b.65-PMMU: minimal TC enable/disable control.
 *
 * 6b.64 still fails despite no explicit translated data access, but retains
 * scratch setup, register preservation and a NOP while TC is active. Remove
 * all of those variables. This is the minimum PMMU transition: install SRP,
 * flush, enable TC, immediately disable TC, flush, and return success.
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
	pmove	%a1@,%tc
	pmove	%a1@(4),%tc
	pflusha
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
