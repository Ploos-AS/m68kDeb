# m68kDeb

**Debian for classic Motorola 68k systems**

m68kDeb is a Debian-based installation and preservation environment for MMU-equipped Motorola 680x0 computers. Initial target families are **Commodore Amiga**, **Atari 68k**, and **Apple Macintosh 68k**.

The project has two equal goals:

1. Install a current Debian Ports/m68k userspace where practical.
2. Preserve and make historical Debian/m68k releases easy to install on supported classic hardware.

## Hardware policy

- Minimum CPU target: **68020 with MMU** (for example 68020 + 68851 PMMU).
- 68030, 68040 and 68060 systems are first-class targets.
- EC CPUs without an MMU are out of scope.
- FPU is not a general minimum requirement.
- 68020+MMU is supported by policy even when a faster CPU is strongly recommended.

## Initial platforms

- Amiga
- Atari
- Macintosh 68k

The installer architecture keeps the common installation core separate from platform-specific bootstrap and boot handling.

## Debian release scope

Historical releases targeted for qualification:

- Debian 2.0 **Hamm**
- Debian 2.1 **Slink**
- Debian 2.2 **Potato**
- Debian 3.0 **Woody**
- Debian 3.1 **Sarge**
- Debian 4.0-era m68k material only where it can be qualified as an unofficial historical target

Current target:

- Debian Ports **m68k**

Historical releases are preservation targets and must not be presented as secure systems for exposure to untrusted networks.

## Design principles

- One installer model across Amiga, Atari and Macintosh where possible.
- Platform-specific bootstraps may start from the machine's native OS.
- Release behavior is data-driven through manifests rather than hard-coded release forks.
- Reproducible inputs, checksums and provenance are required.
- Network installation is preferred for current Debian.
- Historical installation must remain possible from archived/mirrored media when upstream infrastructure changes.
- Emulator qualification comes before physical-hardware qualification, but does not replace it.
- Qualified work lands on `main`; avoid permanent development branches.

## Project layout

```text
docs/               project, support and qualification documentation
releases/           Debian release manifests
platforms/
  amiga/             Amiga bootstrap/profile data
  atari/             Atari bootstrap/profile data
  mac68k/            Macintosh 68k bootstrap/profile data
installer/           shared installer implementation (later milestones)
tests/               host-side validation and manifest tests
```

## Milestones

See [ROADMAP.md](ROADMAP.md).

M0 establishes scope, provenance rules, release/platform feasibility and the initial machine support policy. It intentionally does **not** implement an installer yet.

## Status

**M0 — Foundation & Feasibility: implemented.**

See [docs/M0_FOUNDATION.md](docs/M0_FOUNDATION.md) and [docs/SUPPORT_MATRIX.md](docs/SUPPORT_MATRIX.md).
