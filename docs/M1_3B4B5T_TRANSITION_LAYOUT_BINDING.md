# M1.3b.4b.5t — runtime transition layout binding

M1.3b.4b.5t is a reversible FS-UAE/AROS qualification milestone.

It binds the symbolic layout of the already-qualified MC68030 transition trampoline to the actual runtime page proven physical==logical by the TC-disabled identity-window contract. The trampoline is copied and byte-verified as in 5r/5s, but the real transition entry is **not executed**.

## PASS requires

- relocation-free trampoline object;
- exact offsets for `entry`, `mmu_off`, `post_mmu`, `tc_zero`, and `end` extracted from the trampoline symbols;
- `entry < mmu_off < post_mmu < tc_zero < end`;
- blob size is non-zero and at most one 4 KiB page;
- the runtime page is owned by an AROS `MemHeader`;
- `TC.E == 0`;
- the allocated page passes the 5p identity-window contract using the exact blob size;
- the copied blob matches byte-for-byte;
- every critical symbolic address has equal logical and physical runtime addresses;
- the copied `tc_zero` datum is four zero bytes;
- the probe returns normally to AROS.

## Hard boundary

5t does not call or jump to the real transition entry. It does not write CACR or TC, does not execute `PFLUSHA`, does not invoke the irreversible commit path, and does not jump to Linux.

A PASS proves that the complete transition layout, including the first instruction, MMU-off instruction boundary, post-MMU fetch point, and TC-zero datum, is bound to one exact runtime physical==logical address space. Execution of the actual transition remains a later milestone.
