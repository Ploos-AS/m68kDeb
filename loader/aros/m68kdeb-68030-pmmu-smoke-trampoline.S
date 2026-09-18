/*
 * M1.3b.4b.6b-PMMU: translated data-read control.
 *
 * int entry(const ULONG srp[2], ULONG alias_page, UWORD *stage);
 *
 * Instruction fetch under active TC and PTESTR controls return, while a write
 * to the identity-mapped stage word hangs. Isolate read translation from the
 * write/M-bit path: read stage while TC is active, then disable TC and report
 * whether the expected 0xffff sentinel was observed.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a1		/* srp */
	movel	12(%sp),%a0		/* stage */
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a2
	pmove	%a1@,%srp
	pflusha
	pmove	%a2@,%tc
	movew	%a0@,%d1		/* translated data read */
	pmove	%a2@(4),%tc
	pflusha
	moveq	#0,%d0
	cmpw	#0xffff,%d1
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
