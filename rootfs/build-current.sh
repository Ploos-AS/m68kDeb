#!/bin/sh
set -eu

OUT=${1:-out/rootfs-current-m68k.tar.xz}
WORK=${M68KDEB_WORK:-out/rootfs-current}
MIRROR=${M68KDEB_MIRROR:-https://deb.debian.org/debian-ports}
ARCH=m68k
SUITE=unstable

need() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "missing required tool: $1" >&2
        exit 2
    }
}

need mmdebstrap
need tar
need xz

mkdir -p "$(dirname "$OUT")" "$WORK"

# The build is intentionally explicit about both Debian Ports suites.  The
# bootstrap starts from unstable and makes unreleased available for porter
# packages needed by m68k.  Signature checking is left enabled; callers must
# provide a host with the Debian Ports archive keyring installed.
mmdebstrap \
    --architectures="$ARCH" \
    --variant=minbase \
    --include=apt,ca-certificates,debian-ports-archive-keyring,ifupdown,iproute2,openssh-server \
    --components=main \
    --customize-hook='printf "%s\n" \
      "deb https://deb.debian.org/debian-ports unstable main" \
      "deb https://deb.debian.org/debian-ports unreleased main" \
      > "$1/etc/apt/sources.list"' \
    "$SUITE" \
    "$WORK" \
    "$MIRROR"

# Normalize a few archive inputs so repeated builds are easier to compare.
# Full bit-reproducibility will be qualified in a later milestone once package
# snapshots are pinned rather than using the rolling Ports archive directly.
tar --numeric-owner --sort=name -C "$WORK" -c . | xz -T0 -9e > "$OUT"

echo "rootfs: $OUT"
sha256sum "$OUT"
