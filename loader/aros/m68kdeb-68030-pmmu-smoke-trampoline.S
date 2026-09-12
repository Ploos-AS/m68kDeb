/*
 * M1.3b.4b.6b-PMMU: relocation-free MC68030 dual-alias PMMU smoke trampoline.
 *
 * int entry(const ULONG srp[2], ULONG logical_alias_entry);
 *
 * The caller maps this copied page twice: once at its real physical address
 * (identity mapping) and once at a low linked/logical alias. This mirrors the
 * Linux 68030 transition more closely than the original smoke test:
 *
 *   physical PC -> enable TC -> jump to logical alias -> jump back through
 *   identity alias -> disable TC -> return.
 *
 * No stack or external-memory access occurs while translation is enabled.
 */

	.text
	.balign 4
	.globl m68kdeb_68030_pmmu_smoke_entry
	.globl m68kdeb_68030_pmmu_smoke_alias
	.globl m68kdeb_68030_pmmu_smoke_end

m68kdeb_68030_pmmu_smoke_entry:
	.chip	68030
	movel	4(%sp),%a0
	movel	8(%sp),%a1
	lea	%pc@(m68kdeb_68030_pmmu_smoke_physical_resume),%a2
	lea	%pc@(m68kdeb_68030_pmmu_smoke_tc),%a3
	pmove	%a0@,%srp
	pflusha
	pmove	%a3@,%tc
	jmp	(%a1)

	/* Keep the logical alias at a fixed page offset for the C harness. */
	.org	64
m68kdeb_68030_pmmu_smoke_alias:
	/* TC is still enabled here. Return via the identity alias first. */
	jmp	(%a2)

m68kdeb_68030_pmmu_smoke_physical_resume:
	/* We are executing through the physical identity alias again. */
	pmove	%a3@(4),%tc
	pflusha
	moveq	#0,%d0
	rts

	.balign 4
m68kdeb_68030_pmmu_smoke_tc:
	.long	0x82c07760
	.long	0x00000000

m68kdeb_68030_pmmu_smoke_end:
