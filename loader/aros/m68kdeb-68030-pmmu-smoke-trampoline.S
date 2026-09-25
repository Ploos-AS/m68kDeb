/*
 * M1.3b.4b.6b.22-PMMU: reproduce Linux TT1 plus logical-alias fetch.
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
	/* 6b.22: reproduce Linux's active Amiga 68030 Zorro III TT1 while the\n\t * low logical alias is translated through SRP. Precompute the logical alias target,\n\t * the physical return target,
	 * and the physical address of the zero TC image while translation is off.
	 * After the first instruction fetch through the logical alias, immediately
	 * disable TC through the already-qualified identity mapping and return.
	 * This removes the alias->identity jump while TC is active from 6b.20. */
	lea	%pc@(1f),%a3
	lea	%pc@(m68kdeb_68030_pmmu_smoke_entry),%a4
	subl	%a4,%a3
	addl	%a2,%a3
	lea	%pc@(2f),%a4
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc+4),%a5
	pmove	%a1@,%tc
	jmp	%a3@
1:	moveq	#0x2a,%d1
	pmove	%a5@,%tc
	pflusha
	jmp	%a4@
2:	moveq	#0,%d0
	cmpl	#0x2a,%d1
	beq	3f
	moveq	#1,%d0
3:
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	/* E=1, SRE=1, FCL=0, PS=12, TIA=7, TIB=7, TIC=6. */
	.long	0x82c07760
	.long	0x00000000
	.long	0x13579bdf

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
