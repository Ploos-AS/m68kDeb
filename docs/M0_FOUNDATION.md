# M0 — Foundation & Feasibility

## Goal

M0 freezes the initial contract for m68kDeb before installer implementation begins.

m68kDeb is an installation and preservation environment for Debian on classic MMU-equipped Motorola 68k computers. It is not initially a fork of the Debian package archive.

## Decisions

### CPU baseline

The project baseline is **68020 + MMU**. 68020 with external PMMU, 68030, 68040 and 68060 are in scope. 68000/68010 and EC variants lacking an MMU are out of scope. 68020+MMU is a real support target, although slow configurations may receive strong performance warnings. FPU is not a universal minimum.

### Platform families

Initial families are Commodore Amiga, Atari 68k and Apple Macintosh 68k. The shared installer must not assume one firmware, partition table, disk controller or bootstrap model.

### Release model

Historical official m68k targets: Debian 2.0 Hamm, 2.1 Slink, 2.2 Potato, 3.0 Woody and 3.1 Sarge. Debian 4.0-era m68k work is an unofficial historical investigation target. Current Debian Ports/m68k is the modern target.

A listed release is an intended target, not a claim that every release/platform/machine combination is already qualified.

### Installer architecture

```text
native OS / platform bootstrap
            |
            v
   Linux kernel + initramfs
            |
            v
     shared m68kDeb core
       /      |       \
    Amiga    Atari    Mac68k
            |
            v
      release backend
   historical / current
```

Historical releases may use original release-specific installation mechanics where necessary. Fidelity and reliability take priority over forcing every Debian generation through identical mechanics.

### Provenance and preservation

For redistributed or cached upstream artifacts, retain where available: original/archive origin, filename, size, cryptographic checksum, acquisition date, upstream signature information and relevant license/copyright metadata. Preservation copies of original Debian artifacts should remain byte-identical.

Historical systems must be clearly labelled obsolete and unsafe for routine exposure to untrusted networks.

### Qualification vocabulary

- **SUPPORTED** — intended by project policy.
- **QUALIFIED** — reproduced successfully under a documented procedure.
- **UNVERIFIED** — plausible/intended but not yet tested.
- **UNSUPPORTED** — intentionally out of scope.
- **BLOCKED** — intended but prevented by a known dependency/problem.

Emulator qualification and physical-hardware qualification are recorded separately.

## M0 exit criteria

M0 passes when project scope, 68020+MMU policy, three initial platform families, release classes, manifest format, provenance rules, qualification terminology and the roadmap to a first current-Debian Amiga installer are committed.

M0 deliberately makes no blanket runtime PASS claim.
