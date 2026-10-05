/*
 * M1.3b.4b.6b.91-PMMU: isolate post-TC SRP write from ATC flush.
 *
 * 6b.89 passes the full matrix with PMOVE TC followed by a translated long
 * jump. 6b.90 adds PMOVE SRP plus PFLUSHA after that jump and fails FS-UAE,
 * FS-UAE 3.2.35, and the generic smoke runtime. Keep the post-TC SRP write
 * but remove only the following PFLUSHA. PASS implicates the flush; FAIL
 * implicates the SRP write itself or the translated state immediately after it.
 *
 * The post-TC SRP write reloads the same known-good root pointer, so no page
 * table topology change is introduced by this probe.
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
	pmove	%a0@,%srp
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
