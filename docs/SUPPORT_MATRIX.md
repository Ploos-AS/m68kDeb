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

## Platforms

| Platform | Historical Debian | Current Debian Ports | M0 status |
|---|---:|---:|---|
| Amiga | target | target | SUPPORTED / UNVERIFIED |
| Atari 68k | target | target | SUPPORTED / UNVERIFIED |
| Macintosh 68k | target | target | SUPPORTED / UNVERIFIED |

## Releases

| Release | Codename | Class | m68kDeb intent |
|---|---|---|---|
| 2.0 | Hamm | historical official | preserve/install |
| 2.1 | Slink | historical official | preserve/install |
| 2.2 | Potato | historical official | preserve/install |
| 3.0 | Woody | historical official | preserve/install |
| 3.1 | Sarge | historical official | preserve/install |
| 4.0 era | Etch-era m68k | unofficial | investigate individually |
| current Ports | sid/Ports | current unofficial port | primary modern target |

## Candidate machine classes

These are later qualification candidates, not M0 runtime PASS claims.

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

Exact models are added only after kernel/device/boot-path verification.
