# M1.3b.4b.5m — bounded transparent-translation descriptor read

M1.3b.4b.5m adds the first target-side reader that may dereference a descriptor
address, but only after M1.3b.4b.5l has produced an exact transparent-translation
identity proof for the same address and span.

## Contract

The bounded reader accepts only descriptor-sized reads:

- 4 bytes for a short descriptor; or
- 8 bytes for a long descriptor.

The proof must state that the logical and physical addresses are identical,
transparent translation is active for the candidate, and the admitted byte span
matches the requested read exactly.

Any mismatch fails closed before memory is touched.

After validation, the provider converts the already-proven logical address to a
volatile byte pointer and copies exactly the admitted 4 or 8 bytes. It performs
no wider access and exposes no general physical-memory primitive.

## Safety boundary

5m does not execute PTEST, does not write TC/CRP/SRP/TT0/TT1, does not flush the
ATC, does not disable the MMU, and does not enter the takeover commit path. The
pointer dereference is gated by a pre-existing 5l proof and is limited to the
exact descriptor span.

CI host tests exercise proof and size rejection without dereferencing arbitrary
addresses. The Amiga cross-build and disassembly qualify the target-only read
implementation statically. Runtime execution in FS-UAE remains a later step.

PASS therefore proves the bounded provider contract and target build only. It
does not prove that a particular AROS runtime supplies a usable TT identity
mapping, and it does not authorize MMU shutdown or a Linux jump.
