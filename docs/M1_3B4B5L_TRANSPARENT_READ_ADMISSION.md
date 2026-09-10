# M1.3b.4b.5l — MC68030 transparent-translation read admission

M1.3b.4b.5l defines the admission rule for treating a physical descriptor
address as a directly readable logical address while the MC68030 MMU remains
enabled.

The MC68030 transparent-translation path uses the logical address directly as
the physical address when a transparent-translation register matches. 5l does
not configure TT0 or TT1. It only consumes status evidence from a separate,
reversible PTEST qualification step.

## Admission contract

A read candidate is accepted only when all of the following are true:

1. PTEST status reports transparent translation;
2. PTEST status reports no bus error, limit violation, supervisor violation or
   invalid translation;
3. the logical address proposed for the read equals the physical address that
   must be read;
4. the requested byte span is non-zero; and
5. the byte span does not wrap the 32-bit address space.

An accepted result is an identity-read proof for exactly that address and byte
span. It is not a general proof that surrounding memory or the whole machine is
identity mapped.

## Safety boundary

5l does not execute PTEST, does not write TT0/TT1, TC, CRP or SRP, does not
flush translation state, and does not dereference the admitted address. It only
validates already-collected status and address evidence.

The next runtime milestone may use this admission result to permit a bounded
4-byte or 8-byte descriptor read through the existing 5k provider interface.
If transparent identity cannot be demonstrated for the descriptor address, the
provider must fail closed.

PASS therefore proves the admission logic only. It does not prove that the
current AROS/FS-UAE runtime actually provides a transparent identity mapping at
a particular descriptor address, and it does not authorize MMU shutdown or a
Linux jump.
