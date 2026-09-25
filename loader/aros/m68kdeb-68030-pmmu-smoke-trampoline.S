/*
 * M1.3b.4b.6b-PMMU: same-page data read with FCL disabled.
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
	movel	8(%sp),%a2		/* logical alias of this trampoline */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1
	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc
	/* 6b.19: mirror Linux: after TC enable, transfer execution to the
	 * separately mapped logical alias before doing any further work. */
	lea	%pc@(1f),%a3
	lea	%pc@(m68kdeb_68030_pmmu_smoke_entry),%a4
	subl	%a4,%a3
	addl	%a2,%a3
	jmp	%a3@
1:	movel	%a1@(8),%d1		/* supervisor-data read, same mapped page */
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
