# M1.3b.4b.5y — reversible pre-commit readiness

M1.3b.4b.5y is the final reversible readiness gate before any deliberately irreversible MC68030 transition experiment.

It reuses the production loader unchanged through an interposition harness. The harness builds the real Linux/m68k payload layout, bootinfo and initramfs takeover request, proves their physical==logical identity under the runtime TC state, allocates an Exec-owned 4 KiB trampoline page, proves that page is identity-qualified, copies the exact relocation-free transition blob, and verifies the copy byte-for-byte.

The workflow derives and checks the exact offsets of the real transition symbols: `entry`, `mmu_off`, `post_mmu`, `tc_zero`, `abi_probe`, `reversible_probe`, and `end`. Runtime qualification binds those offsets to the copied logical/physical page and verifies the copied `tc_zero` word remains four zero bytes.

The only copied code executed is the already-qualified reversible ABI probe. It must capture the exact request ABI used by the isolated commit implementation: `a0=&request`, `a1=entry`, `a2=bootinfo`, and `a3=initramfs`, then return normally to AROS.

A PASS emits `M68KDEB_5Y_PRECOMMIT_READY` and requires normal return to AROS.

The irreversible boundary remains closed in this milestone. The real transition entry is not called, `m68kdeb_takeover_68030_commit` is not linked or called, CACR and TC are not written, PFLUSHA is not executed, and Linux is not entered. Static CI checks and runtime markers explicitly preserve these exclusions.
