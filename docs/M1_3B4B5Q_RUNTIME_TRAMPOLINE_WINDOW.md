# M1.3b.4b.5q — runtime trampoline identity window

M1.3b.4b.5q qualifies one actual page-aligned AROS allocation as a candidate physical==logical trampoline window on the MC68030 runtime profile.

The probe allocates 8191 bytes from MEMF_PUBLIC, derives a 4 KiB-aligned page inside that allocation, locates the Exec MemHeader that contains the complete page, reads TC in supervisor state using the read-only 5o helper, and submits the complete 4 KiB page to the 5p identity-window validator.

PASS requires TC.E=0, logical_base==physical_base, 4096-byte alignment, full-page containment in the owning MemHeader, and a normal return to Startup-Sequence.

This milestone intentionally does not copy or execute the irreversible trampoline blob. It does not disable caches or the MMU, flush the ATC, invoke takeover commit, or jump to Linux. The full 4 KiB page is admitted so a later milestone can copy the already-qualified <=4 KiB trampoline blob into it without weakening the placement proof.
