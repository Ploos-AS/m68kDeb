# M1.3b.4b.5j — MC68030 translation evidence resolver

M1.3b.4b.5j combines the reversible evidence produced by the 5h PTEST probe
with the terminal page-descriptor decoder qualified in 5i.

This milestone remains fully reversible. It does not disable or reconfigure the
MMU, does not flush translation state, does not enter the takeover commit path,
and does not jump to Linux.

## Inputs

The resolver consumes:

- the tested logical address;
- the 16-bit MC68030 PTEST/MMU status value;
- the physical address of the last descriptor returned by PTEST;
- a caller-supplied page shift;
- an explicitly identified short or long terminal page-descriptor format; and
- the descriptor word(s) read for that terminal descriptor.

The status masks match the MC68030 MMU status register layout used by Linux/m68k:
B=0x8000, L=0x4000, S=0x2000, WP=0x0800, I=0x0400, M=0x0200,
T=0x0040 and N=0x0007.

## Admission rules

Evidence is accepted only when:

1. the last-descriptor physical address is non-zero;
2. PTEST reports no bus error, limit violation, supervisor violation or invalid translation;
3. PTEST reports at least one table level traversed; and
4. the supplied descriptor is accepted by the 5i terminal page decoder.

Write-protect and modified state are preserved as metadata; they do not prevent
address resolution by themselves.

## Safety boundary

5j does not itself read physical memory at the PTEST-returned descriptor address.
It therefore does not yet prove that the supplied descriptor words came from the
reported physical location. A later runtime-qualified stage must perform that
read safely and bind the bytes to the PTEST evidence before the resulting
physical address can be admitted to the 5g identity-window validator.

PASS proves deterministic evidence admission and composition only. PASS does
not prove an identity mapping, safe MMU shutdown, Linux entry state, or Linux
early boot.
