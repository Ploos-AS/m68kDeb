/*
 * M1.3b.4b.6b.74-PMMU: clear transient non-zero GPR before translated read.
 *
 * 6b.73 fails when d2=1 is established after PMOVE TC and remains non-zero
 * for the translated same-page data read. Clear d2 again before that read.
 * PASS means the GPR value must be non-zero when translated data is accessed;
 * FAIL means even a transient non-zero GPR while TC is active is sufficient.
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
	moveq	#1,%d2
	moveq	#0,%d2
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
	/* E=1, SRE=1, FCL=0, PS=12, TIA=7, TIB=7, TIC=6. */
	.long	0x82c07760
	.long	0x00000000
	.long	0x13579bdf

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
