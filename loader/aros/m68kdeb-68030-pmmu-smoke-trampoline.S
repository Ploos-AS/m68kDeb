/*
 * M1.3b.4b.6b.95-PMMU: ordinary read through external a0 under active TC.
 *
 * 6b.93 passes when active SRP is read to local storage and written back.
 * 6b.94 fails current FS-UAE, FS-UAE 3.2.35, and generic smoke after adding a
 * post-TC PMOVE from the original external a0 operand. Replace only that PMOVE
 * with an ordinary longword read through the same a0 address.
 *
 * PASS says the translated a0 data path itself works and implicates PMOVE's
 * memory operand semantics. FAIL says the external a0 address/data translation
 * is already sufficient to reproduce the problem.
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
	movel	%d2,%d3
	moveq	#0,%d2
	pmove	%a1@,%tc
	jmp	1f
1:
	pmove	%srp,%a1@(12)
	pmove	%a1@(12),%srp
	movel	%a0@,%d2
	movel	%d3,%d2
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
	.long	0x00000000
	.long	0x00000000

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
