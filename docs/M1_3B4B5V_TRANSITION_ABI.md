# M1.3b.4b.5v — reversible transition ABI qualification

M1.3b.4b.5v proves the exact register state that the existing isolated 68030 commit path stages immediately before its irreversible boundary, but does so through a separate reversible entry inside the already-qualified relocation-free trampoline blob.

## Register contract

The existing commit code loads:

- `a0` = pointer to `struct m68kdeb_takeover_request`
- `a1` = `req.entry_addr`
- `a2` = `req.bootinfo_addr`
- `a3` = `req.initramfs_addr`

The 5v-only probe additionally receives `a4` as a caller-owned capture buffer. It stores a0-a3, returns zero, and executes `RTS`.

## PASS requires

- trampoline remains relocation-free and at most one 4 KiB page;
- the 5v ABI probe symbol is inside the copied blob and before the existing 5s four-byte reversible probe;
- the existing 5s reversible probe remains exactly `70 00 4e 75` and exactly four bytes long;
- the runtime page is inside an Exec `MemHeader` and passes the TC-disabled physical==logical identity-window contract;
- the full trampoline blob copies byte-for-byte;
- the copied ABI probe is called, returns normally, and captures exactly the expected request pointer, Linux entry, bootinfo address and initramfs address;
- static gates reject privileged cache/MMU instructions or branches in the ABI-probe slice.

## Hard boundary

5v does not execute the real transition entry. It does not mask interrupts, write CACR or TC, execute `PFLUSHA`, invoke the isolated commit path, or jump to Linux.

A PASS proves that the exact a0-a3 handoff state can be established and observed at an executable offset inside the physical==logical copied trampoline, while remaining fully reversible.
