/*
 * M1.3b.4b.6b.94-PMMU: compare SRP operand addresses under active TC.
 *
 * 6b.91 fails when the post-TC SRP image is loaded directly from the external
 * qualifier pointer in a0. 6b.93 passes the full matrix when the active SRP is
 * first read into local translated storage and that identical image is written
 * back. Preserve the proven read/write-back, then perform a second SRP write
 * from the original a0 operand. Both images should describe the same known-good
 * root. A failure therefore points at the operand/data-address path rather than
 * the SRP value or SRP writes in general.
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
	pmove	%a0@,%srp
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
