/*
 * M1.3b.4b.6b.75-PMMU: isolate transient non-zero GPR with no translated data access.
 *
 * 6b.74 clears d2 again before a translated same-page data read. Remove that
 * read entirely: enable TC, make d2 transiently non-zero, clear it, then
 * disable TC immediately. FAIL means transient unrelated GPR state alone is
 * sufficient while translation is active; PASS requires a subsequent
 * translated data access (or another operation) to expose the failure.
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
