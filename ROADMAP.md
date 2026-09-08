# m68kDeb roadmap

## M0 — Foundation & Feasibility

- Define project purpose and non-goals.
- Establish the 68020+MMU minimum CPU policy.
- Define initial Amiga, Atari and Macintosh platform scope.
- Inventory historical Debian/m68k releases.
- Define current Debian Ports target.
- Establish provenance, checksum and archival rules.
- Define release manifest schema and qualification terminology.

## M1 — Current Debian on Amiga

- Build a reproducible current m68k rootfs.
- Define the Amiga bootstrap path.
- Boot installer kernel/initramfs on a qualified Amiga emulator.
- Detect CPU/MMU/RAM/storage/network.
- Implement network installation.
- Install a bootable minimal Debian Ports system.
- Qualify representative 020+MMU, 030, 040 and 060 profiles where emulation permits.

## M2 — Historical Debian on Amiga

- Add Hamm, Slink, Potato, Woody and Sarge release backends.
- Preserve original installer artifacts and checksums/provenance.
- Support archive/offline installation paths.
- Document compatibility differences by release.

## M3 — Atari

- Qualify ARAnyM development/reference profiles.
- Add Atari bootstrap handling.
- Add TT/Falcon-class profiles.
- Qualify current and historical release paths.

## M4 — Macintosh 68k

- Establish emulator/reference hardware matrix.
- Add Macintosh bootstrap handling.
- Add representative II/Centris/Quadra-class profiles.
- Qualify current and historical release paths.

## M5 — Unified installer UX

- Common release selection.
- Hardware report and compatibility checks.
- Minimal/server/development installation profiles.
- Rescue mode.
- Offline media/cache support.

## M6 — Repository & release engineering

- m68kDeb metapackages where justified.
- Signed metadata.
- Reproducible release artifacts.
- Mirrors and long-term preservation integration.
- Physical-hardware qualification reports.
