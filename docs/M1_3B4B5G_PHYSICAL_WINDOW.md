# M1.3b.4b.5g — physical transition window candidate contract

M1.3b.4b.5g remains fully reversible. It does **not** disable the MMU and does not jump to Linux.

The milestone converts the physical==logical transition requirement into an explicit validator that can consume a later MC68030 translation probe result.

A candidate window is accepted only when:

- logical base equals physical base;
- the base obeys a power-of-two alignment requirement;
- the window is large enough to contain the complete qualified 68030 trampoline;
- address arithmetic does not wrap;
- the complete window lies inside a declared RAM region.

The validator deliberately does not invent or infer a physical address. A later milestone must obtain physical-address evidence from a qualified MC68030 translation inspection mechanism before runtime takeover can be authorized.

## PASS means

A 5g PASS proves the admission logic rejects non-identity, misaligned, undersized, overflowing and out-of-RAM candidates and accepts a valid identity-mapped candidate.

## PASS does not mean

A 5g PASS does not prove that any current AROS allocation is physically identity mapped, does not execute privileged MMU changes, and does not prove Linux early boot.

The irreversible path remains forbidden until a later probe can supply trustworthy logical→physical translation evidence and that evidence satisfies this validator.
