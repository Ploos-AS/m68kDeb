/*
 * M1.3b.4b.6b.58-PMMU: isolate translated stack access without RTS.
 *
 * 6b.57 fails with an explicit PEA/JMP/RTS sequence, so JSR decoding is not
 * required. Exercise only a balanced stack write/read while TC is active,
 * without changing control flow. FAIL implicates translated stack memory
 * access; PASS isolates the remaining failure to RTS/control-return semantics.
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
	pmove	%a1@,%tc
	movel	#0x13579bdf,-(%sp)
	movel	(%sp)+,%d1
	pmove	%a1@(4),%tc
	pflusha
	movel	%d3,%d2
	moveq	#0,%d0
	cmpl	#0x13579bdf,%d1
	beq	1f
	moveq	#1,%d0
1:
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

	.balign 4
m68kdeb_68030_pmmu_smoke_target_marker:
	.long	0xcafebabe
m68kdeb_68030_pmmu_smoke_target:
	movel	#0x13579bdf,%d1
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
