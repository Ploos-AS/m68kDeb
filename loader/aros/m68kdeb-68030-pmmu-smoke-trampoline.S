/*
 * M1.3b.4b.6b.102-PMMU: isolate mapped external read after TC.
 *
 * 6b.101 passes current FS-UAE, FS-UAE 3.2.35, and generic smoke with
 * post-TC SRP read/write using local mapped storage. Reintroduce only the
 * ordinary external data read, but point it at the copied SRP image that is
 * explicitly mapped in the successor page rather than the original incoming
 * a0 address or TT1 alias.
 *
 * PASS confirms the earlier failures are address-space/mapping selection,
 * not translated data reads or post-TC SRP writes themselves.
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
	movel	%a1@(20),%d2
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
