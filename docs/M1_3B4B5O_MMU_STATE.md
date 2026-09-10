# M1.3b.4b.5o — MC68030 translation-control runtime state

M1.3b.4b.5o observes the MC68030 Translation Control (TC) register in the
qualified AROS/FS-UAE runtime without changing MMU state.

The TC enable bit (E, bit 31) determines whether normal paged address
translation is enabled. When E is clear, normal logical addresses are used
directly as physical addresses instead of being translated through the ATC or
table tree. Transparent-translation registers remain a separate architectural
mechanism, so this milestone does not infer their contents or configuration.

## Runtime contract

The probe enters supervisor state only long enough to execute one read-only
`PMOVE TC -> memory`, then immediately returns to user state. It reports the
full TC value and classifies the E bit as either:

- `M68KDEB_5O_TRANSLATION_DISABLED`; or
- `M68KDEB_5O_TRANSLATION_ENABLED`.

Either classification is a valid observation. PASS means that TC was read and
the probe returned normally; PASS does not require translation to be disabled.

## Safety boundary

5o never writes TC, CRP, SRP, TT0 or TT1. It performs no PTEST, no PFLUSH, no
cache-control write, no interrupt-mask change, no descriptor dereference, no
takeover commit and no Linux jump.

If runtime reports TC.E=0, the earlier level-7 PTEST result with INVALID status
and no final descriptor is consistent with paged translation being disabled.
That observation may be used by a later milestone to define an identity-access
contract, but 5o itself does not authorize an MMU transition or Linux handoff.
