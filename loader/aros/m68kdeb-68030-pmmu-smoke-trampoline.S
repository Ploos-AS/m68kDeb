/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 PMMU alias-read trampoline.
 *
 * int entry(const ULONG srp[2], ULONG alias, ULONG expected);
 *
 * The descriptor hierarchy is prepared by the C harness.  Keep every stack
 * access before TC is enabled: the minimal PMMU table maps the trampoline and
 * alias pages, not the caller's stack page.  Once translation is active the
 * probe touches only mapped addresses and register state.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030

	/* Load every caller argument before enabling translation. */
	movel	4(%sp),%a0		/* SRP descriptor pair */
	movel	8(%sp),%a2		/* logical alias */
	movel	12(%sp),%d1		/* expected first longword */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a1

	pmove	%a0@,%srp
	pflusha
	pmove	%a1@,%tc

	/* No stack accesses while TC is active. */
	movel	%a2@,%d0
	cmpl	%d1,%d0
	bne	m68kdeb_68030_pmmu_smoke_mismatch

	pmove	%a1@(4),%tc
	pflusha
	moveq	#0,%d0
	rts

m68kdeb_68030_pmmu_smoke_mismatch:
	pmove	%a1@(4),%tc
	pflusha
	moveq	#1,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

m68kdeb_68030_pmmu_smoke_end:
