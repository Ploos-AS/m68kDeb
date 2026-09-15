/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU alias instruction-fetch probe.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * Keep the normal AROS runtime, exception vectors and supervisor stack usable
 * while PMMU translation is enabled by transparently translating the complete
 * lower 2 GiB with TT0.  The test alias is deliberately at 0x80000000, outside
 * that transparent window, so its instruction fetch must use the SRP tables.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030

	/* Resolve every address before translation is enabled. */
	movel	4(%sp),%a0		/* SRP descriptor pair */
	movel	8(%sp),%a2		/* high-half logical alias page */
	movel	12(%sp),%a4		/* stage word on physical trampoline page */
	addl	#(m68kdeb_68030_pmmu_alias_entry-m68kdeb_68030_pmmu_smoke_entry),%a2
	lea	%pc@(m68kdeb_68030_pmmu_physical_resume),%a3
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tt0),%a5

	/* Clear caches, then make all A31=0 accesses physical/transparent. */
	movel	#0x00000808,%d0
	movec	%d0,%cacr
	pmove	%a5@,%tt0
	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc
	movew	#0x1111,%a4@

	/* 0x80000000 is outside TT0: this fetch must walk the PMMU tables. */
	jmp	%a2@

m68kdeb_68030_pmmu_alias_entry:
	movew	#0x2222,%a4@
	jmp	%a3@

m68kdeb_68030_pmmu_physical_resume:
	movew	#0x3333,%a4@
	pmove	%a1@(4),%tc
	pflusha
	pmove	%a1@(4),%tt0
	movel	#0x00000808,%d0
	movec	%d0,%cacr
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

	/*
	 * MC68030 TT0: base=0x00, address mask=0x7f, E=1, RWM=1,
	 * FC mask=7.  Only A31 must match zero, so the lower 2 GiB is
	 * transparently translated for every function code and read/write access.
	 */
m68kdeb_68030_pmmu_smoke_tt0:
	.long	0x007f8107

m68kdeb_68030_pmmu_smoke_end:
