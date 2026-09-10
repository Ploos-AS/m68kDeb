# M1.3b.4b.5h — reversible MC68030 translation evidence probe

M1.3b.4b.5h adds an isolated, read-only PMMU inspection primitive for MC68030 systems. It does **not** disable the MMU, alter TC/CRP/SRP/TT registers, flush the ATC, or transfer control to Linux.

The probe executes `PTESTW #5,(An),#7,Am` in supervisor state and then reads `MMUSR`. Function code 5 selects supervisor data space. The returned address-register value is the physical address of the last translation-table descriptor accessed by the table search; it is not, by itself, the translated physical address of the tested logical byte.

The probe therefore returns two pieces of evidence:

- the MC68030 `MMUSR` result for the tested logical address;
- the physical address of the last descriptor accessed during the walk.

This milestone deliberately stops there. A later milestone must decode the translation descriptors, account for page/table geometry and logical-page offset, and derive the final physical address before the M1.3b.4b.5g identity-window validator may accept a candidate.

## Static qualification requirements

The isolated object must:

- assemble for `-m68030`;
- contain `PTESTW` and a read from `MMUSR`;
- contain no write to `TC`, `CRP`, `SRP`, `TT0`, `TT1` or `MMUSR`;
- contain no `PFLUSH`/`PFLUSHA`;
- contain no reference to the irreversible takeover commit symbol;
- return normally to its caller.

## Runtime qualification boundary

5h is compile/disassembly qualification only. It does not yet execute the privileged probe under FS-UAE/AROS. Runtime execution requires a guarded supervisor-mode harness and explicit evidence that the emulated 68030 PMMU implements the required PTEST/MMUSR behavior.

A 5h PASS therefore proves the probe primitive is isolated and non-mutating by construction; it does not yet prove a real allocation is identity mapped or that Linux can safely be entered.
