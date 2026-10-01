/*
 * M1.3b.4b.6b.59-PMMU: isolate translated stack write.
 *
 * 6b.58 fails with a balanced push/pop while TC is active, proving that JSR,
 * JMP and RTS are not required. Perform only one predecrement stack write
 * while translation is active. Disable TC before restoring SP and validate
 * the stored value afterwards. FAIL implicates the translated stack write;
 * PASS would isolate the 6b.58 failure to the translated stack read/pop.
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
	pmove	%a1@(4),%tc
	pflusha
	movel	(%sp)+,%d1
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
m68kdeb_68030_pmmu_smoke_end:
