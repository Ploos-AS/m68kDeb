# M1.3 build/static qualification

Status: **PASS**

This report qualifies only the reproducible construction and static validation of the M1.3 Amiga boot bundle. It does not qualify execution under Amiga hardware or an emulator.

## Qualified baseline

- Repository: `Ploos-AS/m68kDeb`
- Branch: `main`
- Qualified commit: `9e307ec9b58fac5656b39679856058920a870f1e`
- Workflow: `M1.3 Amiga boot artifacts`
- Workflow run: `34259220026` (#12)
- Job: `build`
- Result: `success`

All workflow steps completed successfully:

1. checkout;
2. build dependency installation;
3. Amiga Linux kernel build;
4. installer initramfs build;
5. Amiga boot loader fetch and verification;
6. M1.3 bundle assembly and static checks;
7. artifact upload.

## Pinned/recorded inputs

Linux source:

```text
version: Linux 7.2.4
sha256: 01710ee01737dac492f1bae52becd057e08d20d11589089aa06accff415c28dd
file: linux-7.2.4.tar.xz
```

Amiga boot loader:

```text
version: amiboot 5.6
sha256: 8aae977164de07dea64857a6aa6a47c2bf8cc02dc3bde0052c42975cb7e0df9d
```

BusyBox package resolved from Debian Ports for this qualified run:

```text
sha256: 4fb7f78eb996019f2a484ec6b4ec5b5ccf2103961c18d678eb1091ff85ed9bc5
file: busybox-static_m68k.deb
```

The Linux and amiboot inputs are fail-closed against repository-pinned expected SHA-256 values. The Debian Ports BusyBox package hash is captured as qualification provenance for this run.

## Qualified bundle hashes

The uploaded bundle contains:

```text
8aae977164de07dea64857a6aa6a47c2bf8cc02dc3bde0052c42975cb7e0df9d  amiboot
b858724c555a952edfbfdfe7e9ed783bce5d0c84efbd2d47573a9535cfd8822a  vmlinux-m68k-amiga
32628612bdef6e20fd0df0e9973fbb5a8a8bf34089cae7f8deb1dc12c91072b1  vmlinuz-m68k-amiga
18c85e759e08979e7d393db470a79121f5e2c2e3045bb28c78f871f989e11c2a  initramfs-m68kdeb.gz
716b54b2735982d5865eb7fc86f74f48f48e2c26ccd7efdbf2ad3a6d9ea9023e  Start-m68kDeb
14a624ae9f86785d40784d6d8518bcd3ff10cc1f8811c691b10de49193535a8b  Start-m68kDeb-Serial
cff5527391e1ec402b72d144e6f569d495d5c5df803efb27255f4f9e68e30795  PROVENANCE.txt
```

The launchers use the uncompressed `vmlinux-m68k-amiga`, matching the `amiboot 5.6` bootstrap requirement. The compressed kernel is retained separately for preservation and possible future bootstrap variants.

## GitHub Actions artifact

- Artifact ID: `10069609236`
- Artifact name: `m1-3-amiga-boot-9e307ec9b58fac5656b39679856058920a870f1e`
- Size: `7,530,497` bytes
- ZIP digest: `sha256:164f2c9b40e473d33b23be8d82b72d4820fbbe337b642216d34c0ddea9d48d5d`
- Created: `2026-09-08T17:59:38Z`
- Expiry recorded by GitHub Actions: `2026-09-22T17:59:37Z`

The downloaded artifact was inspected after the run and its included checksum/provenance files matched the values recorded above.

## Qualification boundary

PASS means that the repository can construct the defined M1.3 Amiga boot bundle from integrity-checked primary boot inputs, produce the expected kernel/initramfs/bootstrap artifacts, validate the bundle statically, and publish it as a GitHub Actions artifact.

It does **not** mean that Linux has booted on an Amiga or in FS-UAE.

The following remain UNVERIFIED at this milestone:

- execution of `amiboot` under the chosen AmigaOS environment;
- AROS Kickstart compatibility with the bootstrap path;
- Linux kernel entry and console output in FS-UAE;
- successful execution of `/init`;
- observation of `M1.3 bootstrap reached userspace.`;
- real-hardware qualification.

## Next milestone

M1.3a will attempt a redistributable public-CI runtime qualification using:

```text
GitHub Actions -> Xvfb -> FS-UAE -> AROS Kickstart -> amiboot -> Linux 7.2.4 -> m68kDeb initramfs -> PASS marker
```

If AROS Kickstart is technically incompatible with this path, M1.3a must record that limitation rather than silently substituting a proprietary ROM.
