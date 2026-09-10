# M1.3b.4b.5i — MC68030 terminal page descriptor decoder

M1.3b.4b.5i remains fully reversible and host-testable. It does not execute PTEST, does not enter supervisor mode, does not mutate MMU state, and does not jump to Linux.

The milestone adds a deliberately narrow decoder for a **known terminal page descriptor**. The caller must already know:

- whether the terminal descriptor is short or long format;
- the page shift derived from the qualified translation-control state;
- that the supplied descriptor is the terminal descriptor reached by the translation walk.

The decoder does not walk pointer tables and does not infer descriptor length from arbitrary memory. It rejects descriptor-type values other than the terminal page encoding (`DT=01`).

For a short-format terminal page descriptor, the page-address bits are taken from the descriptor word itself. For a long-format terminal page descriptor, the page-address bits are taken from the second long word. The page shift determines the page mask; the logical page offset is then combined with the decoded physical page base.

This follows the MC68030 descriptor model documented in the Motorola/NXP MC68030 User's Manual, section 9.5.1, including short- and long-format page descriptors and the low-order descriptor-type field.

## PASS means

A 5i PASS proves that the pure decoder:

- accepts known short and long terminal page descriptors;
- computes page base, logical page offset and candidate physical address deterministically;
- rejects invalid, pointer/non-page, unsupported-format and invalid-page-shift inputs;
- builds both on the host and for the m68k Amiga target.

## PASS does not mean

A 5i PASS does not prove that the descriptor supplied by PTEST is itself the terminal page descriptor, does not perform a physical table walk, does not prove the current AROS mapping is identity mapped, and does not authorize MMU shutdown.

A later milestone must combine the 5h PTEST/PSR evidence with qualified physical descriptor reads/table-walk logic. Only after a complete translation yields a trustworthy physical address may that result be supplied to the 5g physical-window validator.
