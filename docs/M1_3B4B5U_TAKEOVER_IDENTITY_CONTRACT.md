# M1.3b.4b.5u — takeover identity contract

M1.3b.4b.5u is a reversible FS-UAE/AROS qualification milestone.

It extends the physical==logical proof from the transition trampoline to the complete address set carried by `struct m68kdeb_takeover_request`: kernel layout, kernel entry, bootinfo, and initramfs.

## PASS requires

- the ordinary reversible `m68kdeb_takeover_68030_prepare()` contract passes;
- runtime `TC.E == 0`;
- the kernel layout and immediately-adjacent bootinfo span do not overflow and are wholly contained in one reported Exec `MemHeader` RAM range;
- the initramfs span does not overflow and is wholly contained in a reported Exec `MemHeader` RAM range;
- under disabled paged translation, logical and physical addresses are equal for layout base, kernel entry, bootinfo and initramfs;
- the runtime probe returns normally to AROS.

The runtime probe uses representative AROS allocations to qualify the address contract itself. It does not yet bind a real built kernel/initramfs payload to this proof; that remains a later integration milestone.

## Hard boundary

5u does not execute the real transition entry. It does not write CACR or TC, execute `PFLUSHA`, invoke the irreversible commit path, or jump to Linux.

A PASS establishes that the full takeover request can be expressed as bounded RAM-backed physical==logical addresses under the currently-qualified MC68030 runtime state. It is a prerequisite for binding the actual materialized kernel/bootinfo/initramfs payloads to the same contract.
