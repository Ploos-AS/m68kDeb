/*
 * M1.3b.4b.6b.99-PMMU: read external a0 through the 0x40000000 alias.
 *
 * 6b.98 did not assemble because OR-immediate cannot target an address register. 6b.95 fails on a direct post-TC read through the original external a0
 * address, while 6b.97 passes when the same data is copied locally before TC
 * and read PC-relative afterwards. Linux's 68030 setup provides the Zorro III
 * 0x40000000 transparent-translation window. Form that alias for the external
 * operand and dereference it after TC.
 *
 * PASS shows the external storage remains reachable through the intended TT1
 * alias and turns the earlier failure into an address-space selection issue.
 * FAIL means the external operand is not rescued by the Linux-like alias and
 * needs explicit mapping/placement analysis.
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
	movel	%a0@,%a1@(20)
	movel	%d2,%d3
	moveq	#0,%d2
	pmove	%a1@,%tc
	jmp	1f
1:
	pmove	%srp,%a1@(12)
	pmove	%a1@(12),%srp
	movel	%a0,%d2
	orl	#0x40000000,%d2
	movel	%d2,%a2
	movel	%a2@,%d2
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
	.long	0x00000000

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
