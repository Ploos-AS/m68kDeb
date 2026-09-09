# M1.3b.4b — privileged takeover and first Linux entry

M1.3b.4b is the first irreversible m68kDeb bootstrap step. It begins only after M1.3b.4a has proved the final Linux kernel image layout and bootinfo placement while still returning safely to AROS.

The goal of this milestone is deliberately narrow: quiesce the AROS/Amiga execution environment sufficiently for Linux/m68k entry, transfer control to the already-qualified kernel entry address, and obtain unambiguous evidence that Linux early boot has executed. It is not yet a full Debian installer or hardware qualification milestone.

## Preconditions

The takeover path must refuse to run unless all reversible checks have already succeeded:

- Linux image is ELF32, big-endian, m68k.
- The kernel advertises the Amiga bootinfo ABI version expected by the loader.
- Every `PT_LOAD` segment has been materialized into the final contiguous layout.
- BSS/tails are zero-filled.
- Kernel entry lies inside the materialized image.
- Bootinfo follows the kernel image immediately and self-validates through `BI_LAST`.
- Initramfs is represented by `BI_RAMDISK` with non-zero address and size.
- CPU/MMU policy accepts the running machine.
- The final destination ranges do not overlap loader-owned state required before the jump.

If any precondition fails, the loader must remain on the reversible path and return to AROS with a diagnostic. No partial takeover is allowed.

## Irreversible boundary

The implementation must have an explicit final boundary immediately before machine takeover. Everything before that boundary remains ordinary AROS code and must be able to abort cleanly.

After crossing the boundary the code may perform only the operations required to establish the Linux entry environment. The exact sequence is CPU-family-sensitive, but the implementation must account for:

- entering supervisor context;
- disabling maskable interrupts;
- preventing further AROS scheduling/interrupt activity;
- quiescing DMA-capable Amiga hardware where required by the Linux bootstrap contract;
- placing cache state in the form expected by early Linux/m68k code;
- disabling or replacing the active MMU translation state as required by the target CPU;
- establishing a known stack/scratch environment that cannot be overwritten before the jump;
- preserving the already-built kernel image, bootinfo and initramfs ranges;
- transferring control to the qualified Linux entry address with the Amiga boot protocol state expected by the kernel.

There is no supported return to AROS after the irreversible boundary.

## CPU-family policy

M1.3b.4b should be implemented and qualified incrementally rather than pretending one takeover sequence is correct for every 680x0 CPU.

The initial CI target remains the known-good 68030 + MMU A1200 profile. 68020+68851, 68040 and 68060 takeover differences are deferred to the M1.5 qualification matrix unless they are required to keep the implementation structurally correct.

CPU-specific cache/MMU handling must be isolated behind explicit routines so later 68020/030/040/060 qualification does not require rewriting the generic loader/layout code.

## Required runtime evidence

A PASS must prove more than disappearance from AROS. At minimum the workflow must capture deterministic Linux-side evidence after the jump, for example serial/debug output emitted by Linux early boot and a marker from the m68kDeb initramfs.

Required evidence should include:

- reversible preflight PASS;
- final-layout PASS;
- an explicit `M68KDEB_TAKEOVER_COMMIT` marker emitted immediately before the irreversible boundary;
- Linux/m68k early console output after kernel entry;
- Linux machine identification consistent with Amiga;
- evidence that the supplied initramfs was discovered;
- a deterministic initramfs marker such as `M68KDEB_INITRAMFS_STARTED`.

The workflow must classify timeout, emulator reset, exception before Linux output, or loss of evidence as FAIL rather than PASS.

## CI qualification profile

Keep the M1.3b.4a public profile for the first attempt:

- Ubuntu 24.04 runner;
- FS-UAE under Xvfb;
- pinned matching AROS m68k ROM/system;
- generic A1200;
- 68030 + MMU;
- real CPU speed;
- 64 MiB Zorro III FastRAM;
- Linux 7.2 Amiga kernel;
- m68kDeb initramfs;
- no proprietary ROM or accelerator firmware.

The takeover workflow must upload serial/stdout logs, emulator log, provenance, kernel/initramfs hashes, and at least one screenshot when practical.

## PASS boundary

M1.3b.4b is PASS only when the loader performs the irreversible handoff and the same CI run captures Linux-side execution evidence. A successful jump with no Linux evidence is not sufficient.

## Explicit non-claims

M1.3b.4b does not yet prove:

- complete hardware discovery;
- network installation;
- persistent disk installation;
- booting the installed Debian system without the AROS bootstrap;
- 68020+68851, 68040 or 68060 correctness;
- physical Amiga hardware support.

Those belong to M1.4 and M1.5.
