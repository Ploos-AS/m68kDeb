/*
 * M1.3b.4b.6b.79-PMMU: control pre-TC instruction layout with NOP.
 *
 * 6b.78 fails when moveq #0,d2 is placed immediately before PMOVE TC.
 * Replace only that two-byte instruction with a two-byte NOP, preserving the
 * PMOVE TC and active-window offsets. PASS means register-write/state is the
 * trigger; FAIL means the added instruction/layout itself is sufficient.
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
	nop
	pmove	%a1@,%tc
	nop
	pmove	%a1@(4),%tc
	pflusha
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	/* E=1, SRE=1, FCL=0, PS=12, TIA=7, TIB=7, TIC=6. */
	.long	0x82c07760
	.long	0x00000000
	.long	0x13579bdf

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
