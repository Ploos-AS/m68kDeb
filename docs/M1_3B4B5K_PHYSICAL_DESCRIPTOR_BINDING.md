# M1.3b.4b.5k — physical descriptor binding contract

M1.3b.4b.5k binds descriptor bytes to the exact physical address reported by
the reversible MC68030 PTEST evidence path, without assuming that a physical
address can be dereferenced as a normal logical pointer while the MMU is active.

The binder therefore accepts an explicit physical-read provider. The provider
must either return the requested bytes from the exact physical address and byte
count, or fail. The binder itself performs no MMU reconfiguration and no raw
physical pointer dereference.

## Admission rules

- the PTEST last-descriptor address must be non-zero and 32-bit aligned;
- short terminal descriptors request exactly 4 bytes;
- long terminal descriptors request exactly 8 bytes;
- reader failure is propagated and no evidence is accepted;
- descriptor bytes are assembled as big-endian 32-bit words;
- the resulting words are passed to the 5j translation-evidence resolver;
- only successful 5j evidence becomes bound translation evidence.

## Safety boundary

5k does not implement a hardware physical-memory reader. In particular it does
not cast the PTEST-returned physical address to a pointer, does not modify TC,
CRP, SRP or transparent-translation registers, does not flush the ATC, does not
enter the takeover commit path, and does not jump to Linux.

A later milestone must qualify a concrete reversible physical-read provider for
the target runtime. Until then, 5k proves address/length binding and evidence
composition, not that AROS can directly access the reported physical address.
