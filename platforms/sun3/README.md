# Sun-3 / Sun-3x platform

Sun 68k workstations are an initial m68kDeb platform family.

## Initial targets

### Sun-3

Linux has a dedicated Sun-3 machine/MMU implementation for 68020 systems using Sun's custom MMU. Initial targets are:

- Sun 3/50
- Sun 3/60 — primary reference target
- Sun 3/1xx family
- Sun 3/2xx family

The Sun 3/60 is the first emulator target because active QEMU development exists for this machine and has demonstrated firmware boot plus network boot of other operating systems. This QEMU support is not yet treated as a released dependency.

### Sun-3x

Linux also retains `CONFIG_SUN3X` for 68030 Sun-3x systems, but the kernel labels the support very experimental.

Initial experimental target:

- Sun 3/80

Current MAME Sun-3x emulation is useful as a development reference but is marked not working and is not sufficient for qualification PASS.

## Debian status

The current Debian m68k port identifies Sun-3 as an m68k Linux subarchitecture for which kernels and boot images would be useful, rather than a currently complete installer target. m68kDeb therefore expects to own the Sun-specific kernel/boot qualification work.

Historical Debian release support must be established from evidence per release. The presence of a Debian m68k userspace does not by itself imply historical Sun-3 installer support.

## Qualification policy

Sun qualification is split into independent gates:

1. Linux kernel boot on the target model.
2. Console and timer stability.
3. Storage or network-root path.
4. Ethernet where available.
5. Debian Ports userspace boot.
6. Installer/bootstrap path.
7. Emulator PASS, then physical hardware PASS when hardware is available.

No Sun target is runtime-qualified merely by being listed here.
