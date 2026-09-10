# M1.3b.4b.5n — reversible runtime descriptor-read qualification

M1.3b.4b.5n moves the 68030 translation work into FS-UAE/AROS runtime while
preserving the reversible boundary.

## Runtime chain

The probe performs these steps:

1. enter supervisor state only for the privileged PTEST instruction;
2. PTEST a known logical address and record its PSR plus the physical address of
   the last translation-table descriptor accessed;
3. return to user state;
4. independently PTEST the descriptor physical value as a logical address;
5. return to user state again;
6. require the second PTEST result to report transparent translation with no
   translation fault;
7. bind logical == physical for exactly four bytes through the 5l admission
   contract; and
8. permit the 5m bounded reader to copy exactly four descriptor bytes.

The second PTEST is mandatory. The PSR from the original target translation is
not evidence that the returned descriptor physical address is directly readable
as a logical pointer.

## Fail-closed rule

If the runtime does not transparently map the descriptor physical address, the
probe reports `M68KDEB_5N_NOT_ADMITTED` and performs no descriptor dereference.
Such a result is a qualification failure for the end-to-end read path, not a
reason to weaken the admission rule.

## Safety boundary

5n executes PTEST and temporarily uses supervisor state because PTEST is a
privileged MC68030 PMMU instruction. It does not write TC, CRP, SRP, TT0 or TT1,
does not flush the ATC, does not alter cache state, does not disable the MMU,
does not enter the takeover commit path and does not jump to Linux.

A PASS proves only that this specific AROS/FS-UAE 68030 runtime allowed the
independently admitted bounded descriptor read. It does not yet prove descriptor
format or page geometry, Linux entry register state, hardware quiescence, safe
MMU shutdown, or successful Linux early boot.
