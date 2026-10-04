/*
 * M1.3b.4b.6b.78-PMMU: control one zero GPR write before TC.
 *
 * 6b.77 fails when a single moveq #0,d2 executes while translation is active.
 * Move the identical instruction immediately before PMOVE TC and keep the
 * active-TC window otherwise empty. PASS isolates the emulator regression to
 * executing the ordinary GPR write while TC is enabled; FAIL means the write
 * also perturbs the subsequent PMMU enable transition.
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
	moveq	#0,%d2
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
