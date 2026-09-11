# M1.3b.4b.5z transition containment

M1.3b.4b.5z is the first deliberately non-reversible MC68030 transition-runtime qualification. It is restricted to the FS-UAE/AROS CI profile and does not change the production loader.

The qualifier repeats the actual-payload gates established by 5y: real kernel/initramfs takeover request, TC read, logical==physical payload proof while TC.E=0, identity-qualified 4 KiB trampoline page, byte-for-byte copy verification, exact transition symbol ordering, and the reversible a0/a1/a2/a3 ABI probe.

Only after those checks pass does the harness flush its report, create `SYS:m1-3b4b5z-transition-armed.marker`, and jump through the CI-only wrapper into the exact copied `m68kdeb_takeover_68030_trampoline_entry` at offset zero. The wrapper stages the same request ABI as the isolated final commit path.

The transition entry itself is unchanged. It clears CACR, writes TC=0, executes PFLUSHA, and then loops locally. There is no Linux jump in this trampoline. The workflow treats any return to AROS as failure and requires the emulator process to remain alive for a bounded containment interval after the armed marker appears.

This milestone therefore proves that the fully-qualified transfer can be attempted inside the emulator and remains non-returning/contained. It does **not** claim host-observable proof that every individual privileged instruction completed; that distinction is retained explicitly in the evidence.

Safety boundary:

- production loader unchanged;
- `m68kdeb_takeover_68030_commit` is not linked;
- transition execution is limited to GitHub Actions FS-UAE;
- no Linux entry transfer occurs;
- unexpected return is a hard failure.
