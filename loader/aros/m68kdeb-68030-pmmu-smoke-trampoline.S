/*
 * M1.3b.4b.6b.89-PMMU: exercise Linux-like post-TC long-jump control flow.
 *
 * 6b.87 and 6b.88 show that the isolated 6b.85 NOP+NOP failure is not a
 * general translated-instruction-count or register-state limit. Return to the
 * Linux 68030 head.S boundary: after enabling TC, transfer control with a
 * long jump to a nearby translated label before restoring d2 and disabling TC.
 * This tests the control-flow shape that the real kernel uses immediately
 * after PMOVE TC while preserving the restored qualifier envelope.
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

	.balign 4
m68kdeb_68030_pmmu_smoke_end:
