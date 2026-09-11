# M1.3b.4b.6a — Linux transfer static qualification

## Purpose

5z qualified the exact irreversible MC68030 containment sequence at runtime:
cache disable, TC=0, PFLUSHA, then a local non-returning loop. 6a introduces
the smallest possible delta toward Linux: a separate, CI-only/static
trampoline whose post-PFLUSHA action is exactly `jmp (%a1)`.

This milestone does **not** execute the Linux transfer. It makes the final
transfer instruction and its ordering mechanically reviewable before runtime.

## Register contract

The already-qualified transition ABI is preserved:

- `a0` — takeover request
- `a1` — Linux ELF entry address
- `a2` — Amiga bootinfo address
- `a3` — initramfs address

The 6a blob does not alter `a1`, `a2`, or `a3` before transfer.

## Required ordering

The static gate requires these symbols in strict order:

`entry < mmu_off < post_mmu < linux_jump < tc_zero < end`

The code path is therefore:

1. `CACR=0`
2. `TC=0`
3. `PFLUSHA`
4. `JMP (a1)`

## Safety boundary

6a is deliberately separate from the production loader and from the proven
5z containment trampoline. It is relocation-free and contains no call, return,
stack access, or alternate executable transfer on the irreversible path.

`linux_jump=STATIC_ONLY`

The next milestone may bind this exact blob to the actual 5y-qualified payload
and attempt the transfer under the isolated FS-UAE qualification environment.
