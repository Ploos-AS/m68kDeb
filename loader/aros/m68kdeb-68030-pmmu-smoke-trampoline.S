/*
 * M1.3b.4b.6b.77-PMMU: isolate one zero GPR write while TC is active.
 *
 * 6b.76 fails with two moveq #0,d2 instructions under active translation,
 * while the restored historical baseline passes with one same-page data read.
 * Reduce the delta to exactly one zero-valued GPR write. FAIL means a single
 * unrelated GPR write is sufficient in this envelope; PASS means the repeated
 * write/sequence length in 6b.76 is material.
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
