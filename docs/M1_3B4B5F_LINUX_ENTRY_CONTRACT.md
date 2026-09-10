# M1.3b.4b.5f — Linux/m68k entry contract

M1.3b.4b.5f is a static qualification milestone. It does **not** execute the irreversible takeover path and it does **not** disable the MMU at runtime.

## Goal

Close the second blocker identified by M1.3b.4b.5e: qualify the exact Linux/m68k kernel entry relationship that a later physical transition trampoline must use.

## Contract

For the qualified Amiga kernel image:

- the ELF entry address must lie inside a loadable PT_LOAD segment;
- the loader's materialized entry address is `layout_base + (elf_entry - min_load_vaddr)`;
- bootinfo begins immediately after the materialized kernel image, matching the existing m68kDeb handoff contract;
- the kernel image must advertise the Amiga bootinfo interface version expected by the loader;
- the trampoline must remain relocation-free and self-contained;
- 5f may calculate and record the future physical jump target, but must not contain an executable jump to it.

## Required evidence

The qualification gate records:

- ELF entry address;
- minimum and maximum PT_LOAD virtual addresses;
- entry offset from the minimum load address;
- image span;
- expected materialized entry offset;
- Amiga bootinfo magic/version presence;
- trampoline span and relocation count;
- classification stating that Linux entry transfer remains disabled.

## PASS does not prove

A 5f PASS does **not** prove:

- physical==logical placement under AROS;
- safe MMU/cache shutdown at runtime;
- DMA/device/scheduler quiescence;
- correct runtime register state after translation is disabled;
- successful transfer into Linux early boot.

Those remain later gates.

## Runtime rule

No normal loader or CI runtime path may execute the physical transition trampoline as part of 5f. This milestone is evidence for a later guarded transition, not authorization to perform it.
