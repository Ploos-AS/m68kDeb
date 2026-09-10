# M1.3b.4b.5x — actual payload + copied transition ABI qualification

M1.3b.4b.5x combines the two strongest reversible proofs established by 5v and 5w.

The qualification binary includes the production `m68kdeb-loader.c` unchanged and interposes only the completed `m68kdeb_build_takeover_request()` call. The normal loader therefore constructs the real Linux kernel layout, bootinfo and initramfs request exactly as before.

After that exact request is built, 5x:

- verifies the complete real layout + bootinfo span is inside one Exec `MemHeader`;
- verifies the complete real initramfs span is inside one Exec `MemHeader`;
- reads MC68030 `TC` in supervisor state and immediately returns to user state;
- requires the already-qualified 5u actual-payload identity contract to pass under `TC.E=0`;
- allocates a separate page-aligned public RAM page for the transition trampoline;
- qualifies that whole page as logical == physical with the 5p/5g identity-window contract;
- copies the real relocation-free transition trampoline blob into that page and byte-verifies it;
- enters only the copied 5v reversible ABI probe, never the real transition entry;
- supplies the exact real takeover request using the final commit register convention: `a0=request`, `a1=Linux entry`, `a2=bootinfo`, `a3=initramfs`;
- requires the copied probe to capture exactly those four values and return normally;
- returns through the production loader, which must still report `handoff=NOT_ATTEMPTED`.

## PASS contract

A PASS requires the visible matching-AROS A1200/68030 FS-UAE runtime to show all of the following:

- actual kernel and initramfs loaded;
- `abi_tc_rc=0` and `abi_tc=0x00000000`;
- `actual_payload_identity_rc=0`;
- `trampoline_identity_rc=0`;
- `M68KDEB_5X_TRAMPOLINE_COPY_VERIFIED`;
- `actual_payload_abi_rc=0`;
- captured `a0` equals the exact takeover-request pointer;
- captured `a1` equals the exact real Linux entry address;
- captured `a2` equals the exact real bootinfo address;
- captured `a3` equals the exact real initramfs address;
- exact logical == physical equality for the actual layout, entry, bootinfo and initramfs;
- `M68KDEB_5X_ACTUAL_PAYLOAD_ABI_BOUND`;
- `M68KDEB_TAKEOVER_PREFLIGHT_OK`;
- `handoff=NOT_ATTEMPTED`;
- `M68KDEB_5X_RETURNED`.

## Hard boundary

M1.3b.4b.5x is still fully reversible. It does **not** execute `m68kdeb_takeover_68030_trampoline_entry`, alter CACR, write TC, issue PFLUSHA as part of takeover, mask interrupts for commit, or jump to Linux. The irreversible commit object is not linked into this qualification binary.

A green 5x means the real payload addresses, physical identity proof, copied trampoline placement and final register ABI have all been demonstrated together without crossing the irreversible boundary.
