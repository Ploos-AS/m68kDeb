/*
 * M1.3b.4b.6b.101-PMMU: isolate post-TC SRP write with mapped operand.
 *
 * 6b.100 keeps the incoming SRP image in the explicitly mapped successor
 * page, but still combines a post-TC SRP write with an external/TT1 data
 * probe and fails the real runtimes. Remove that external access completely.
 * The only substantive post-jump PMMU operation retained is readback of SRP
 * to local mapped storage followed by writing the identical image back.
 *
 * FAIL strongly isolates PMOVE memory->SRP while translation is active.
 * PASS would instead implicate the external data/alias access from 6b.100.
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
