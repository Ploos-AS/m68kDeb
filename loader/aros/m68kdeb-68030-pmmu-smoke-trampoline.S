/*
 * M1.3b.4b.6b.26-PMMU: isolate first post-TC fall-through fetch.
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
	movel	8(%sp),%a2		/* logical alias of this trampoline */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1
	pmove	%a0@,%srp
	pflusha
	/* 6b.26 control: remove every post-TC jump. If this returns, the first
	 * translated fall-through fetch is sound and the minimal failure boundary
	 * is the control transfer performed while TC is active. */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc+4),%a5
	pmove	%a1@,%tc
	moveq	#0x2a,%d1
	pmove	%a5@,%tc
	pflusha
	moveq	#0,%d0
	cmpl	#0x2a,%d1
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
