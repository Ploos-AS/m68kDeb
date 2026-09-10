# M1.3b.4b.5w — actual payload identity qualification

M1.3b.4b.5w proves that the **actual takeover request produced by the real loader path** is physically identical to its logical addresses in the qualified A1200/MC68030 FS-UAE runtime where `TC.E=0`.

The qualification harness includes `m68kdeb-loader.c` unchanged and interposes only `m68kdeb_build_takeover_request()`. The normal loader therefore still performs the real kernel load, ELF inspection, final PT_LOAD layout allocation/materialization, bootinfo construction, initramfs allocation, takeover-request construction, and reversible takeover preflight.

After the real request has been constructed, 5w:

- finds the Exec `MemHeader` containing the complete layout + bootinfo span;
- finds the Exec `MemHeader` containing the complete initramfs span;
- enters supervisor state only long enough to read MC68030 `TC` with the already-qualified 5o helper;
- returns to user state immediately;
- requires `TC.E=0`;
- calls the already-qualified 5u takeover identity validator on the actual request;
- requires exact logical == physical equality for layout, Linux entry, bootinfo, and initramfs;
- returns through the production loader, which must still print `handoff=NOT_ATTEMPTED`.

## PASS contract

A PASS requires all of the following from the visible FS-UAE A1200/68030 matching-AROS runtime:

- real Amiga Linux kernel loaded;
- real m68kDeb initramfs loaded;
- `identity_tc_rc=0`;
- `identity_tc=0x00000000`;
- `takeover_identity_rc=0`;
- `M68KDEB_TAKEOVER_IDENTITY_OK`;
- `M68KDEB_TAKEOVER_PREFLIGHT_OK`;
- exact identity equality for layout, entry, bootinfo, and initramfs;
- `handoff=NOT_ATTEMPTED`;
- `M68KDEB_BOOTINFO_BUILD_OK`;
- loader returns to AROS.

## Hard boundary

M1.3b.4b.5w remains fully reversible. It does **not** execute the real transition entry, disable caches, alter the MMU, issue a takeover PFLUSH, or jump to Linux. The isolated irreversible commit object is not linked into the qualifier.

The purpose of 5w is to close the gap between earlier representative-allocation identity proofs and the exact addresses produced by the real kernel/initramfs loader path. Only after this is green should the actual payload be combined with the already-qualified copied transition ABI probe.
