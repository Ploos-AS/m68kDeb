#!/bin/sh
set -eu

MIRROR=${M68KDEB_MIRROR:-https://deb.debian.org/debian-ports}
ARCH=${M68KDEB_ARCH:-m68k}

need() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "missing required tool: $1" >&2
        exit 2
    }
}

need curl
need gzip

check_suite() {
    suite=$1
    echo "== $suite =="
    curl -fsSL "$MIRROR/dists/$suite/InRelease" >/dev/null

    if curl -fsSL "$MIRROR/dists/$suite/main/binary-$ARCH/Packages.gz" \
        | gzip -dc \
        | grep -qm1 '^Package: '; then
        echo "PASS: $suite/main has $ARCH packages"
    else
        echo "FAIL: no readable $ARCH Packages index for $suite/main" >&2
        exit 1
    fi
}

check_suite unstable
check_suite unreleased

echo "PASS: Debian Ports preflight for $ARCH"
