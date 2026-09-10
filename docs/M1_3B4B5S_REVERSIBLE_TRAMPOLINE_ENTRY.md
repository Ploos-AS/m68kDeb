# M1.3b.4b.5s — reversible copied trampoline entry

M1.3b.4b.5s is a reversible FS-UAE/AROS runtime qualification milestone.

It extends the already-qualified relocation-free MC68030 trampoline blob with a dedicated `m68kdeb_takeover_68030_trampoline_reversible_probe` entry. That entry is part of the same copied blob but is isolated from the real transition entry and contains only `moveq #0,d0` followed by `rts`.

The runtime probe repeats the 5q/5r evidence chain: it allocates a real 4 KiB-aligned AROS page, finds the owning Exec `MemHeader`, reads TC without modifying it, admits the page only when `TC.E == 0`, copies the complete trampoline blob, verifies it byte-for-byte, then calls only the copied reversible-probe offset and requires a normal return.

## PASS requires

- the trampoline object remains relocation-free and no larger than one 4 KiB page;
- the reversible probe symbol lies inside the raw blob and after the transition data;
- disassembly of the reversible probe contains only the qualified return stub and no privileged/mutating instruction;
- the runtime allocation is admitted by the 5p identity-window contract;
- the complete copied blob matches byte-for-byte;
- control flow enters the copied reversible probe and returns with `d0 == 0`;
- the process returns normally to AROS.

## Hard boundary

5s does **not** call `m68kdeb_takeover_68030_trampoline_entry`. It does not write CACR or TC, does not execute PFLUSHA, does not invoke the irreversible takeover commit path and does not jump to Linux.

A 5s PASS proves that executable control flow can enter and return from the actual copied identity-mapped trampoline blob. It does not yet authorize crossing the cache/MMU transition boundary.
