# M1.3b.4b.5aa — Linux entry transfer

M1.3b.4b.5aa is the first qualification that intentionally transfers from the copied MC68030 transition trampoline to the actual materialized Linux kernel entry address.

It remains **CI/emulator-only**. The production loader and the isolated production commit object are not changed or linked into this qualification.

## Prerequisites

5aa builds directly on the already-green gates:

- 5o: TC state read and `TC.E=0` under the qualified FS-UAE/AROS profile;
- 5p/5q/5r: bounded identity window, runtime page allocation and copied trampoline verification;
- 5v: reversible request/register ABI proof;
- 5w/5x/5y: actual kernel, bootinfo and initramfs identity plus exact precommit ABI readiness;
- 5z: the real copied transition entry safely crossed CACR/TC/PFLUSHA and remained contained without returning.

## New operation

The separate 5aa trampoline repeats the qualified 5z sequence exactly:

1. `CACR=0`;
2. `TC=0`;
3. `PFLUSHA`;
4. then, and only then, `JMP (A1)`.

`A1` is populated from `req->entry_addr` by the already-qualified transition-entry wrapper. The actual request has already proved `entry_logical == entry_physical` before the irreversible boundary.

The existing production/5z trampoline is deliberately left unchanged; its local halt remains available as the containment reference.

## Runtime evidence

Before the irreversible transfer, the harness persists and flushes evidence for:

- actual payload identity;
- copied 5aa trampoline identity and byte verification;
- exact request/register ABI (`A0=request`, `A1=entry`, `A2=bootinfo`, `A3=initramfs`);
- exact logical/physical Linux entry equality;
- the static offset of the new Linux `JMP` inside the copied blob;
- an on-disk `linux-entry-armed` marker.

The host then requires the loader not to return and FS-UAE to remain alive for a bounded observation window.

## PASS meaning

A PASS qualifies the **Linux entry transfer boundary**: after the same transition path already proven by 5z, the executable copied blob contains and reaches the direct transfer path to the actual Linux entry without returning to AROS during the observation window.

A PASS does **not** yet claim a complete Linux boot, userspace startup, initramfs success, console output, or device initialization. Those require a later gate with positive Linux-owned evidence rather than merely a non-returning transfer.

## Safety boundary

- no production loader handoff is enabled;
- `m68kdeb_takeover_68030_commit()` is not linked or called;
- the test is restricted to the matching A1200/68030 FS-UAE profile;
- failure or emulator instability is a failed qualification, never reclassified as success.
