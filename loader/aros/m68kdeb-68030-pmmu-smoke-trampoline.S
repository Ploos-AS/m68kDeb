/*
 * M1.3b.4b.6b.53-PMMU: compute register-indirect jump target after TC.
 *
 * 6b.52 passes on FS-UAE, FS-UAE 3.2.35 and Amiberry when the target is
 * computed before PMOVE TC and the active-translation jump is register
 * indirect. Move only the PC-relative LEA across PMOVE TC. This separates
 * post-enable effective-address computation from JMP register-indirect itself.
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
	lea	%pc@(1f),%a0
	jmp	%a0@
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
