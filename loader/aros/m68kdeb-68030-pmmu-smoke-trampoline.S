/*
 * M1.3b.4b.6b.76-PMMU: control register writes at zero while TC is active.
 *
 * 6b.75 fails with d2 changing 0 -> 1 -> 0 while TC is active, without any
 * explicit translated data access. Keep the same two register-writing
 * instructions and active-TC lifetime, but write zero both times. PASS means
 * the transient non-zero value is required; FAIL means register-write activity
 * itself is sufficient in this execution envelope.
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
	moveq	#0,%d2
	moveq	#0,%d2
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
