/*
 * M1.3b.4b.6b.42-PMMU: isolate post-TC register write from data read.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * The known-good control keeps the translated execution page identical to
 * the physical trampoline page. 6b.41 moved a non-zero d2 write after TC and
 * then performed the supervisor data read. This variant keeps that write but
 * disables TC immediately afterwards, before any translated data access.
 * PASS means the register write itself is safe and the following data read
 * is the narrower trigger. FAIL means merely changing a general register
 * while TC is enabled is sufficient to reproduce the hang.
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
	/* 6b.42: enable TC, change one general register, then immediately
	 * restore TC=0 through the identity path. No translated data read occurs
	 * while the non-zero register value is live. */
	pmove	%a1@,%tc
	movel	#0x00000001,%d2
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
