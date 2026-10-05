/*
 * M1.3b.4b.6b.93-PMMU: read active SRP and write the exact value back.
 *
 * 6b.91 fails the real runtimes when SRP is written after TC is enabled.
 * 6b.92 replaces that write with an SRP read and passes current FS-UAE,
 * generic smoke, and Amiberry (FS-UAE 3.2.35 retains a separate read-path
 * failure). Read the active SRP into writable storage, then immediately PMOVE
 * that exact image back into SRP while translation remains active.
 *
 * If current runtimes fail again, the write operation itself is implicated
 * even when the SRP value and page-table root are unchanged.
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
