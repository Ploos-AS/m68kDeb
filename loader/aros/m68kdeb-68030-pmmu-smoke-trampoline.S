/*
 * M1.3b.4b.6b.56-PMMU: compare register-indirect JSR after TC.
 *
 * 6b.55 fails on the FS-UAE paths with JSR abs.l while Amiberry passes.
 * Precompute the identical same-page subroutine target before PMOVE TC, then
 * call it through JSR (a0). PASS isolates the FS-UAE regression to absolute-
 * long control-transfer addressing rather than JSR/RTS or stack semantics.
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
	lea	%pc@(m68kdeb_68030_pmmu_smoke_target),%a0
	pmove	%a1@,%tc
	jsr	%a0@
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
