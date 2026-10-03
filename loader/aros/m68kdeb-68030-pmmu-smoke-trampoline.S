/*
 * M1.3b.4b.6b.72-PMMU: re-test non-zero GPR in restored PASS envelope.
 *
 * 6b.68/69 restored a stable PASS by restoring the historical qualifier
 * envelope; 6b.70/71 were comment-only baseline confirmations. Change one
 * runtime variable: force d2=1 immediately before PMOVE TC. FAIL would confirm
 * that non-zero unrelated GPR state is sufficient inside the restored envelope.
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
	moveq	#1,%d2
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
	/* E=1, SRE=1, FCL=0, PS=12, TIA=7, TIB=7, TIC=6. */
	.long	0x82c07760
	.long	0x00000000
	.long	0x13579bdf

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
