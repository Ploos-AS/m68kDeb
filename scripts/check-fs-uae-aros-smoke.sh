#!/bin/sh
set -eu

CONFIG=${1:-ci/fs-uae/m1_3a_aros_smoke.fs-uae}

[ -f "$CONFIG" ] || { echo "missing config: $CONFIG" >&2; exit 1; }

grep -Eq '^amiga_model[[:space:]]*=[[:space:]]*A3000$' "$CONFIG" || {
  echo "M1.3a smoke profile must use A3000 reference model" >&2
  exit 1
}

grep -Eq '^kickstart_file[[:space:]]*=[[:space:]]*internal$' "$CONFIG" || {
  echo "M1.3a smoke profile must use FS-UAE internal AROS Kickstart" >&2
  exit 1
}

if grep -Eiq '(^|[[:space:]=/])(kick[0-9]*\.rom|amiga-os-[0-9]+\.rom|rom\.key)([[:space:]]|$)' "$CONFIG"; then
  echo "proprietary Kickstart reference detected" >&2
  exit 1
fi

printf 'PASS: FS-UAE AROS-only smoke profile\n'
