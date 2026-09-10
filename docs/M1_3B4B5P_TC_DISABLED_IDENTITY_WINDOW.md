# M1.3b.4b.5p — TC-disabled identity transition window

M1.3b.4b.5p converts an already-observed MC68030 TC state into a bounded
physical transition-window candidate.

M1.3b.4b.5o qualified the current AROS/FS-UAE profile with `TC=0x00000000`
and translation disabled. Under that condition, normal logical addresses are
used directly as physical addresses. 5p encodes that condition explicitly
rather than relying on an implicit identity-mapping assumption.

## Admission contract

The function accepts a candidate only when:

1. the TC enable bit is clear;
2. the logical base and byte count are non-zero;
3. physical base is derived exactly from the logical base;
4. the resulting candidate passes the existing M1.3b.4b.5g physical-window
   validator, including alignment, minimum trampoline size, overflow and RAM
   containment checks.

If translation is enabled, 5p fails closed. It does not fall back to PTEST,
transparent translation, or an assumed physical address.

## Safety boundary

5p is pure validation code. It does not read or write TC, CRP, SRP, TT0 or TT1,
does not execute PTEST or PFLUSH, does not dereference the admitted window,
does not enter the takeover commit path, and does not jump to Linux.

The TC value is evidence supplied by the separately-qualified read-only 5o
runtime probe. PASS therefore proves only the deterministic conversion from a
TC-disabled state to a bounded identity-window candidate. It does not yet prove
that the Linux image, bootinfo and trampoline have all been placed inside the
same qualified runtime window.

The next milestone may bind this admission rule to the actual AROS loader
allocation and trampoline span while keeping execution reversible.
