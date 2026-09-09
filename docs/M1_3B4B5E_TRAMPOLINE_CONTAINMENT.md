# M1.3b.4b.5e — MC68030 trampoline containment contract

M1.3b.4b.5e is a compile/static qualification milestone. It does **not** execute the irreversible takeover path.

The purpose is to prove that the complete MC68030 translation-disable tail can exist as one bounded, relocation-free blob before any runtime test attempts to disable translation.

## Required symbols

The isolated trampoline object must export, in strictly increasing address order:

1. `m68kdeb_takeover_68030_trampoline_start`
2. `m68kdeb_takeover_68030_trampoline_entry`
3. `m68kdeb_takeover_68030_trampoline_mmu_off`
4. `m68kdeb_takeover_68030_trampoline_post_mmu`
5. `m68kdeb_takeover_68030_trampoline_end`

`start` and `entry` may share the same address.

## Static invariants

The qualification gate must prove all of the following:

- the object is built specifically for MC68030/68851 MMU instruction forms;
- the trampoline has no unresolved relocations;
- the blob is non-empty and no larger than one 4 KiB page;
- `mmu_off` lies inside the blob;
- `post_mmu` lies strictly after `mmu_off` and inside the blob;
- the TC=0 source datum used by `PMOVE` lies inside the same blob;
- the disassembly contains the expected CACR, PMOVE TC and PFLUSHA instruction forms;
- the current 5e tail halts locally and contains no Linux entry transfer;
- neither the trampoline object nor the irreversible commit object is linked into the normal reversible loader.

## What PASS does not prove

A 5e PASS does **not** prove:

- that an AROS virtual address is identical to its physical address;
- that copying the blob to arbitrary `MEMF_PUBLIC` memory makes it safe after MMU disable;
- that caches/MMU can be disabled safely from the current AROS address space;
- that the Linux/m68k entry ABI or register state is correct;
- that DMA, scheduler, interrupt sources or platform devices are quiesced;
- that Linux early boot is reachable.

These remain blockers for any irreversible runtime test.

## Runtime rule

No CI or normal loader path may execute `PMOVE ...,%tc` until a later milestone has a defensible way to establish or construct a physical==logical execution window and has separately qualified the Linux entry protocol.
