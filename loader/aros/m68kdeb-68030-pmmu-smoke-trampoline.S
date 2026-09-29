/*
 * M1.3b.4b.6b.51-PMMU: runtime-relocate exact Linux absolute-long jump.
 *
 * 6b.50 exposed that an absolute-long jump cannot be copied as a raw blob without
 * relocating its embedded address. Emit the exact 4ef9 absolute-long opcode
 * with a sentinel operand; the qualifier patches that operand to the copied
 * runtime target before execution. This isolates the Linux jump encoding.
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
	.word	0x4ef9
	.long	0xdeadbeef
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
