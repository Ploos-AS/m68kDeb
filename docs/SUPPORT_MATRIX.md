# Support matrix

This records project intent, not blanket runtime qualification.

## CPU/MMU policy

| CPU class | Status | Notes |
|---|---|---|
| 68000 | UNSUPPORTED | No Linux/m68k MMU target |
| 68010 | UNSUPPORTED | No Linux/m68k MMU target |
| 68020 + MMU/PMMU | SUPPORTED | Project minimum; slow configurations expected |
| 68020 without MMU | UNSUPPORTED | MMU required |
| 68030 | SUPPORTED | First-class target |
| EC030 without MMU | UNSUPPORTED | MMU required |
| 68040 | SUPPORTED | First-class target |
| EC040 without MMU | UNSUPPORTED | MMU required |
| 68060 | SUPPORTED | First-class target |
| EC060 without MMU | UNSUPPORTED | MMU required |

FPU requirements are evaluated per CPU/platform/kernel/release combination.

Sun-3 is a special case: classic Sun-3 systems use a 68020 with Sun's custom MMU rather than a Motorola PMMU. Linux has a dedicated `CONFIG_SUN3` MMU path, so these machines are explicitly in scope even though their MMU implementation differs from Amiga/Atari/Mac 68020 systems.

## Platforms

| Platform | Historical Debian | Current Debian Ports | Project status |
|---|---:|---:|---|
| Amiga | target | target | SUPPORTED / UNVERIFIED |
| Atari 68k | target | target | SUPPORTED / UNVERIFIED |
| Macintosh 68k | target | target | SUPPORTED / UNVERIFIED |
| Sun-3 | investigate | target kernel/boot port | SUPPORTED / UNVERIFIED |
| Sun-3x | investigate | experimental target | SUPPORTED / EXPERIMENTAL |

For Sun systems, `SUPPORTED` means m68kDeb intentionally targets the machine family. It does not imply that Debian Ports currently ships a ready-to-use Sun-3 installer or boot image.

## Releases

| Release | Codename | Class | m68kDeb intent |
|---|---|---|---|
| 2.0 | Hamm | historical official | preserve/install on historically supported platforms |
| 2.1 | Slink | historical official | preserve/install on historically supported platforms |
| 2.2 | Potato | historical official | preserve/install on historically supported platforms |
| 3.0 | Woody | historical official | preserve/install on historically supported platforms |
| 3.1 | Sarge | historical official | preserve/install on historically supported platforms |
| 4.0 era | Etch-era m68k | unofficial | investigate individually |
| current Ports | sid/Ports | current unofficial port | primary modern target |

Historical Sun-3/Sun-3x support is investigated separately; old Debian/m68k releases must not be assumed to support Sun merely because they support the m68k architecture.

## Initial machine targets

These are qualification targets, not blanket runtime PASS claims.

### Amiga

- 68020 expansion with MMU/PMMU
- A2500/20-class
- A2500/30-class
- A3000
- A4000/030 and A4000/040
- A1200/A4000 with 030/040/060 accelerators

### Atari

- TT-class systems
- Falcon-class systems and suitable accelerators
- ARAnyM reference environments

### Macintosh

- MMU-capable Macintosh II-family systems
- Centris-class systems
- Quadra-class systems

### Sun-3 — initial targets

Linux `CONFIG_SUN3` explicitly covers these 68020 + Sun-MMU families:

- Sun 3/50
- Sun 3/60 — **primary Sun reference target**
- Sun 3/1xx family
- Sun 3/2xx family

The Sun 3/60 is the first emulator qualification target because active QEMU work exists for a 3/60 machine capable of firmware boot and network boot. Until that support is merged into a released QEMU version, the emulator path is tracked as upstream-in-progress rather than a released dependency.

### Sun-3x — initial experimental target

Linux `CONFIG_SUN3X` supports the 68030-based Sun-3x architecture but marks it very experimental.

- Sun 3/80 — **initial Sun-3x target**

The MAME Sun 3/80 implementation can execute POST but is currently marked not working and is therefore useful for development/reference work, not qualification PASS.

### Sun systems deferred from the initial qualification set

- Sun 3/460/470/480 — kernel architecture is relevant, but current emulator support is too incomplete for an initial reproducible qualification target.

Exact device-level support, boot media, storage, Ethernet, framebuffer and Debian release compatibility are qualified independently for each model.
