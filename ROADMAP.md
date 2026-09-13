# m68kDeb roadmap

## Emulator / runtime target matrix

m68kDeb explicitly targets multiple emulator families so that qualification can cover both machine-accurate profiles and practical virtualized/runtime-oriented profiles. Emulator qualification and physical-hardware qualification remain separate.

- **FS-UAE** — Amiga reference target; machine-oriented Amiga qualification.
- **Amiberry** — Amiga target; complementary modern Amiga runtime/emulation path.
- **FellowNG** — Amiga target; additional independent Amiga emulator implementation.
- **Hatari** — Atari ST/TT/Falcon target; hardware-oriented Atari qualification.
- **ARAnyM** — Atari-compatible 68040 virtual-machine target; fast development/runtime-oriented qualification.
- **Basilisk II** — Macintosh 68k target; Macintosh runtime/reference qualification.

Support for an emulator does not imply that every CPU, machine or Debian release combination is viable. Each backend gets its own documented capability and qualification matrix as milestones progress.

## M0 — Foundation & Feasibility

- Define project purpose and non-goals.
- Establish the 68020+MMU minimum CPU policy.
- Define initial Amiga, Atari, Macintosh and Sun-3 platform scope.
- Inventory historical Debian/m68k releases.
- Define current Debian Ports target.
- Establish provenance, checksum and archival rules.
- Define release manifest schema and qualification terminology.
- Maintain the explicit emulator/runtime target matrix.

## M1 — Current Debian on Amiga

### M1.1 — Current rootfs

- Build a reproducible current Debian Ports/m68k rootfs.
- Verify Debian Ports `unstable` and `unreleased` m68k package indexes.
- Qualify foreign-arch bootstrap under qemu-user/binfmt.
- Record rootfs SHA-256, size and build commit.

### M1.2 — Amiga bootstrap contract

- Define the `amiboot` handoff from AmigaOS.
- Define required kernel/initramfs/bootstrap bundle shape.
- Provide normal and serial-debug launch templates.
- Add static bundle validation.

### M1.3 — First visible Amiga Linux boot

- Acquire/build a current Amiga-capable m68k kernel.
- Build the m68kDeb installer initramfs.
- Assemble a provenance-recorded bootstrap bundle.
- Boot visibly in FS-UAE.
- Capture serial/debug evidence.

### M1.4 — Hardware detection and network install

- Detect CPU/MMU/RAM/storage/network.
- Implement network installation.
- Install a bootable minimal Debian Ports system.

### M1.5 — Amiga qualification matrix

- Qualify representative 68020+MMU, 68030, 68040 and 68060 profiles where emulation permits.
- Qualify FS-UAE, Amiberry and FellowNG as distinct emulator backends.
- Keep emulator and physical-hardware qualification separate.

## M2 — Historical Debian on Amiga

- Add Hamm, Slink, Potato, Woody and Sarge release backends.
- Preserve original installer artifacts and checksums/provenance.
- Support archive/offline installation paths.
- Document compatibility differences by release.

## M3 — Atari

- Qualify Hatari hardware-oriented development/reference profiles.
- Qualify ARAnyM development/reference profiles as the complementary fast virtual-machine path.
- Add Atari bootstrap handling.
- Add TT/Falcon-class profiles where supported by the selected backend.
- Maintain separate Hatari and ARAnyM capability/qualification matrices.
- Qualify current and historical release paths.

## M4 — Macintosh 68k

- Establish Basilisk II as the initial Macintosh 68k emulator/reference backend.
- Establish emulator/reference hardware matrix.
- Add Macintosh bootstrap handling.
- Add representative II/Centris/Quadra-class profiles where Basilisk II and Linux/m68k support permit.
- Qualify current and historical release paths.

## M5 — Sun-3 / Sun-3x

- Use Sun 3/60 as the primary emulator/reference target as emulator support matures.
- Target Sun 3/50, 3/60, 3/1xx and 3/2xx under Linux `CONFIG_SUN3`.
- Treat Sun 3/80 / Sun-3x as experimental until kernel/emulator qualification is reliable.
- Build and qualify Sun-specific kernel/bootstrap artifacts.
- Qualify current Debian Ports userspace separately from platform kernel/boot support.

## M6 — Unified installer UX

- Common release selection.
- Hardware report and compatibility checks.
- Minimal/server/development installation profiles.
- Rescue mode.
- Offline media/cache support.

## M7 — Repository & release engineering

- m68kDeb metapackages where justified.
- Signed metadata.
- Reproducible release artifacts.
- Mirrors and long-term preservation integration.
- Physical-hardware qualification reports.
