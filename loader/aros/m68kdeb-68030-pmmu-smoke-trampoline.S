/*
 * M1.3b.4b.6b.57-PMMU: isolate JSR from stack/RTS semantics after TC.
 *
 * 6b.56 fails with register-indirect JSR, so absolute-long addressing is not
 * required. Emulate the call explicitly while translation is active: push the
 * return PC, JMP through the precomputed target, then return with RTS. If this
 * also fails, the translated stack write/read or RTS path is implicated; if it
 * passes, JSR itself is the trigger.
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
	pea	%pc@(m68kdeb_68030_pmmu_smoke_return)
	jmp	%a0@
m68kdeb_68030_pmmu_smoke_return:
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
