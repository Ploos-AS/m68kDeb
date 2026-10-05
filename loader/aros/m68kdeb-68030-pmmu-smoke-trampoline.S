/*
 * M1.3b.4b.6b.92-PMMU: read SRP after TC instead of writing it.
 *
 * 6b.89 passes the full matrix after PMOVE TC and a translated long jump.
 * 6b.91 fails the real runtimes when the next PMMU operation writes SRP, even
 * without PFLUSHA. Replace that write with an SRP readback to writable storage.
 * PASS isolates the failure to post-TC SRP writes; FAIL points more broadly at
 * SRP/PMMU access or the translated data access performed by PMOVE.
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
