# M1.3b.4b.5d — physical transition trampoline gate

M1.3b.4b.5c qualified the complete reversible takeover request under the
A1200/68030+MMU FS-UAE/AROS profile.  M1.3b.4b.5d now closes the architectural
gap between that reversible state and a future translation-off transition.

## Safety rule

The irreversible 68030 commit path MUST NOT disable translation while executing
from an address whose physical backing has not been proven to remain reachable
at the same instruction address after MMU translation is removed.

Before any runtime MMU-disable experiment, m68kDeb must therefore use one of:

1. a region proven to be identity mapped (`logical == physical`) for every byte
   executed after the translation-disable boundary; or
2. a small relocation-free transition trampoline copied into a known physical
   address and entered using an execution protocol whose pre/post-MMU addresses
   have been explicitly qualified.

No inference from an Exec virtual address alone is sufficient proof.

## 5d artifact

`loader/aros/m68kdeb-takeover-68030-trampoline.S` is an isolated compile-only
probe.  It establishes the structural requirements for the later physical
transition blob:

- MC68030 code;
- bounded start/end symbols;
- position-independent instructions;
- no relocations;
- no external symbols;
- no cache/MMU manipulation;
- no Linux entry jump;
- not linked into `m68kdeb-loader`.

The probe deliberately does only `moveq #0,d0; rts`.  It is not the future
irreversible trampoline.

## PASS criteria

CI PASS for 5d requires:

- the probe assembles with the same Bebbo/GNU m68k toolchain used by 4b.5;
- start, probe and end symbols exist and are ordered;
- the code span is non-zero and small (<= 256 bytes);
- the object has no relocation records;
- the real reversible loader contains none of the trampoline symbols;
- the existing irreversible commit object remains isolated and unlinked.

## What 5d does not prove

A 5d PASS does **not** prove:

- the physical address of any AROS allocation;
- that current AROS mappings are identity mappings;
- safe runtime MMU disable;
- cache coherency across a copied trampoline;
- supervisor-entry correctness;
- DMA/platform quiescence;
- Linux/m68k entry-register conventions;
- Linux early boot.

Those are later gates.  In particular, the first executable translation-off
experiment must wait until the runtime can determine or construct a qualified
physical transition address without guessing.
