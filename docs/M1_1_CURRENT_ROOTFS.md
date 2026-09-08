# M1.1 — Current Debian Ports rootfs bootstrap

## Objective

Turn the M0 `current` release definition into a host-side, inspectable bootstrap path for a minimal Debian Ports/m68k root filesystem.

M1.1 does **not** claim that the produced rootfs has booted on Amiga yet. Runtime boot qualification belongs to the next Amiga bootstrap milestone.

## Verified upstream state

At implementation time Debian Ports publishes m68k indices in both `unstable` and `unreleased` under `https://deb.debian.org/debian-ports`.

The project policy is:

- bootstrap from `unstable`;
- configure `unreleased` as an additional source for m68k porter packages where needed;
- retain signature verification;
- require `debian-ports-archive-keyring`;
- record exact upstream Release metadata during qualification;
- do not describe a rolling rootfs as bit-reproducible until package inputs are snapshot/pinned.

## Host preflight

Run:

```sh
sh scripts/check-current-ports.sh
```

The check fails closed if either suite lacks a readable `main/binary-m68k/Packages.gz` index.

## Rootfs build

The initial builder uses `mmdebstrap` with architecture `m68k` and the `minbase` variant.

Host requirements:

- `mmdebstrap`
- `debian-ports-archive-keyring`
- `tar`
- `xz`
- whatever foreign-architecture execution support the selected `mmdebstrap` mode requires on that host

Run:

```sh
sh rootfs/build-current.sh
```

Default output:

```text
out/rootfs-current-m68k.tar.xz
```

The builder prints a SHA-256 digest of the resulting archive.

## Initial package profile

The M1.1 minimal target explicitly includes:

- apt
- ca-certificates
- debian-ports-archive-keyring
- ifupdown
- iproute2
- openssh-server

This is a bootstrap/server-oriented profile, not a graphical workstation.

## Reproducibility boundary

The archive construction is normalized for ordering and numeric ownership, but Debian Ports is rolling. A build made on one date can resolve different package versions from a later build.

A later milestone must pin or snapshot package inputs before m68kDeb claims full reproducibility.

## Amiga handoff

M1.2 will define the Amiga-side bootstrap contract:

1. native AmigaOS bootstrap utility;
2. Linux kernel;
3. initramfs;
4. kernel command line;
5. transfer into the shared m68kDeb installer environment;
6. reference machine/emulator profiles.

Debian's documented historical Amiga path uses `amiboot`; m68kDeb will qualify the exact bootstrap binary/version and kernel format rather than assuming historical defaults remain correct for current kernels.

## M1.1 qualification state

- Repository/source metadata: IMPLEMENTED
- Debian Ports network preflight: IMPLEMENTED, runtime execution still required
- Rootfs builder: IMPLEMENTED, runtime execution still required
- m68k rootfs boot on Amiga: NOT YET QUALIFIED

M1.1 is therefore **implementation-complete but runtime-unqualified** until the host commands are executed and their output is recorded.
