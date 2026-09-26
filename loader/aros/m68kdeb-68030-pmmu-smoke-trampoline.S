/*
 * M1.3b.4b.6b.31-PMMU: isolate harmless pre-enable LEA.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * Instruction fetch survives active TC, while reading the stage pointer hangs.
 * Read a constant embedded in the trampoline page instead. This keeps the
 * logical/physical page identical to the executing code and isolates
 * supervisor-data FC/page-walk behaviour from the external stage address.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a0		/* srp */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1
	pmove	%a0@,%srp
	pflusha
	/* 6b.31: sole delta from 6b.27 PASS: compute the same zero-TC address\n	 * into an otherwise unused register before enable. The active-TC sequence\n	 * remains the exact known-PASS a1-relative sequence. */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc+4),%a5
	pmove	%a1@,%tc
	movel	%a1@(8),%d1		/* supervisor-data read, same mapped page */
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
