/*
 * M1.3b.4b.6b.49-PMMU: add Linux-style post-enable long jump.
 *
 * 6b.48 proves the save/restore frame itself is safe. Keep that known-PASS
 * register state and add the next material operation from Linux mmu_engage_030:
 * a long jump immediately after PMOVE TC. The target stays in the same
 * identity-mapped trampoline, isolating post-enable control transfer.
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
	jmp	%pc@(1f)
1:
	movel	%a1@(8),%d1
	pmove	%a1@(4),%tc
	pflusha
	movel	%d3,%d2
	moveq	#0,%d0
	cmpl	#0x13579bdf,%d1
	beq	2f
	moveq	#1,%d0
2:
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000
	.long	0x13579bdf

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
