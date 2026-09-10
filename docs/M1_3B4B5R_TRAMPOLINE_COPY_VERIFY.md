# M1.3b.4b.5r — runtime trampoline copy/verify

M1.3b.4b.5r is a reversible FS-UAE/AROS qualification milestone.

It builds the already statically-qualified MC68030 trampoline as a raw `.text` blob, embeds those bytes in a dedicated runtime probe, allocates a real 4 KiB-aligned AROS page, proves that page is an admitted TC-disabled identity window using the 5p contract, copies the blob into the page, and verifies every byte before returning to AROS.

## PASS requires

- the trampoline object has no relocations;
- the raw trampoline blob is non-empty and no larger than one 4 KiB page;
- the runtime page is fully owned by a reported AROS `MemHeader`;
- `TC.E == 0` at runtime;
- the page passes `m68kdeb_68030_identity_window_from_tc()` with the actual blob length as the required trampoline span;
- `CopyMem()` copies the blob into the admitted page;
- byte-for-byte verification of the copied blob succeeds;
- the probe returns normally to AROS.

## Hard boundary

5r does **not** execute the copied trampoline. It does not write TC, CACR or any MMU register, does not PFLUSH, does not call the irreversible takeover commit path, and does not jump to Linux.

A 5r PASS therefore proves placement and exact byte preservation only. Execution remains blocked until later milestones bind the kernel/bootinfo/initramfs placement and final entry state to the same physical-address contract.
