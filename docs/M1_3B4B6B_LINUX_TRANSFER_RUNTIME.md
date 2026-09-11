# M1.3b.4b.6b — Linux transfer runtime qualification

M1.3b.4b.6b is the first CI-only runtime gate that deliberately transfers control from the AROS loader into the qualified Linux/m68k kernel entry.

## Preconditions

This gate depends on the already qualified chain through 5z and 6a:

- MC68030 `TC.E=0` runtime profile;
- actual kernel/bootinfo/initramfs takeover request identity proof;
- copied trampoline physical==logical placement and byte-for-byte verification;
- exact request/register staging (`a0=req`, `a1=Linux entry`, `a2=bootinfo`, `a3=initramfs`);
- successful 5z execution of `CACR=0 -> TC=0 -> PFLUSHA` with non-return containment;
- 6a relocation-free Linux transfer blob ending in `JMP (a1)`.

## Runtime sequence

The 6b harness includes the production loader unchanged and interposes only the completed takeover-request builder. It then:

1. validates the actual payload identity contract;
2. allocates one 4 KiB identity-qualified trampoline page;
3. copies and byte-verifies the exact 6a Linux-transfer blob;
4. verifies the copied zero-TC data and all critical symbol ordering;
5. writes durable pre-transfer evidence and flushes the report;
6. stages the already-qualified takeover register ABI;
7. transfers into the copied trampoline;
8. executes `CACR=0`, `TC=0`, `PFLUSHA`, then `JMP (a1)` into Linux.

The production commit symbol remains unlinked.

## PASS evidence

A non-return or emulator hang is **not** sufficient. PASS requires positive Linux userspace evidence over the emulated Amiga serial port:

- pre-transfer report contains `M68KDEB_6B_LINUX_TRANSFER_ARMED`;
- no AROS `returned` marker is created;
- serial output contains `m68kDeb installer bootstrap`;
- serial output contains `M1.3 bootstrap reached userspace.` from `/init` in the real initramfs.

FS-UAE exposes the emulated Amiga serial port as a localhost TCP socket. The workflow records the serial transcript as qualification evidence.

## Scope

This remains an emulator-only qualification. A PASS proves that the current A1200/68030/AROS CI profile can make the real irreversible transition and reach Linux initramfs userspace. It does not yet authorize enabling the handoff in the normal production loader or claim coverage for other Amiga models/CPUs.
