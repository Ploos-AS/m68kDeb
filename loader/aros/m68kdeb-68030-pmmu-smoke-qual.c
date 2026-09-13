/* M1.3b.4b.6b-PMMU isolated FS-UAE/AROS MC68030 PMMU qualification. */
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>

#define PAGE_SIZE 4096UL
#define TABLE_RAW_BYTES (32768UL + PAGE_SIZE - 1UL)
#define TRAMP_RAW_BYTES (PAGE_SIZE * 2UL - 1UL)
#define ROOT_TABLE_SIZE 128UL
#define PTR_TABLE_SIZE 128UL
#define PAGE_TABLE_SIZE 64UL
#define ROOT_INDEX_SHIFT 25UL
#define PTR_INDEX_SHIFT 18UL
#define PAGE_INDEX_SHIFT 12UL
#define TABLE_DESC 0x0000000aUL
#define PAGE_DESC 0x00000019UL
#define IDENTITY_LIMIT 0x01000000UL
#define IDENTITY_PTRS (IDENTITY_LIMIT >> PTR_INDEX_SHIFT)
#define LOGICAL_ALIAS_PAGE 0x01000000UL

extern unsigned char m68kdeb_pmmu_smoke_blob[];
extern unsigned int m68kdeb_pmmu_smoke_blob_len;

typedef LONG (*smoke_fn_t)(ULONG *srp, ULONG phase);

static ULONG align_page(ULONG p)
{
    return (p + PAGE_SIZE - 1UL) & ~(PAGE_SIZE - 1UL);
}

static LONG marker(const char *path, const char *text)
{
    BPTR fh = Open((STRPTR)path, MODE_NEWFILE);
    LONG n;
    if (!fh) return 20;
    n = Write(fh, (APTR)text, (LONG)strlen(text));
    Close(fh);
    return n > 0 ? 0 : 20;
}

static LONG run_phase(smoke_fn_t smoke, ULONG *srp, ULONG phase)
{
    APTR old_super;
    LONG rc;

    Disable();
    old_super = SuperState();
    rc = smoke(srp, phase);
    if (old_super) UserState(old_super);
    Enable();
    return rc;
}

int main(void)
{
    UBYTE *table_raw = NULL, *tramp_raw = NULL;
    ULONG table_page, tramp_page, pte_base;
    ULONG *root, *ptr, *pte, *alias_pte;
    ULONG srp[2];
    ULONG ri, pi, ti, lri, lpi, lti;
    ULONG pidx, tidx, phys;
    smoke_fn_t smoke;
    LONG rc = 20;

    Printf("M68KDEB_PMMU_SMOKE_START\n");

    if (!m68kdeb_pmmu_smoke_blob_len || m68kdeb_pmmu_smoke_blob_len > PAGE_SIZE) {
        Printf("FAIL blob_len=%lu\n", (ULONG)m68kdeb_pmmu_smoke_blob_len);
        return 20;
    }

    table_raw = (UBYTE *)AllocMem(TABLE_RAW_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    tramp_raw = (UBYTE *)AllocMem(TRAMP_RAW_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    if (!table_raw || !tramp_raw) {
        Printf("FAIL alloc\n");
        goto out;
    }

    table_page = align_page((ULONG)table_raw);
    tramp_page = align_page((ULONG)tramp_raw);
    root = (ULONG *)table_page;
    ptr = (ULONG *)(table_page + 512UL);
    pte_base = table_page + 1024UL;

    if (tramp_page >= IDENTITY_LIMIT) {
        Printf("FAIL trampoline outside identity window: 0x%08lx\n", tramp_page);
        goto out;
    }

    CopyMem(m68kdeb_pmmu_smoke_blob, (APTR)tramp_page,
            (ULONG)m68kdeb_pmmu_smoke_blob_len);
    CacheClearU();
    smoke = (smoke_fn_t)tramp_page;

    /* Build a complete 0-16 MiB identity map. A one-page identity map is too
     * narrow for a safe PMMU transition because the supervisor stack and any
     * prefetched/runtime accesses must remain valid while TC is enabled. */
    root[0] = ((ULONG)ptr & 0xfffffff0UL) | TABLE_DESC;
    for (pidx = 0; pidx < IDENTITY_PTRS; ++pidx) {
        pte = (ULONG *)(pte_base + pidx * (PAGE_TABLE_SIZE * sizeof(ULONG)));
        ptr[pidx] = ((ULONG)pte & 0xfffffff0UL) | TABLE_DESC;
        for (tidx = 0; tidx < PAGE_TABLE_SIZE; ++tidx) {
            phys = (pidx << PTR_INDEX_SHIFT) | (tidx << PAGE_INDEX_SHIFT);
            pte[tidx] = (phys & 0xfffff000UL) | PAGE_DESC;
        }
    }

    /* The alias lives immediately above the identity window so it exercises
     * a real translated fetch without stealing an address that AROS may use. */
    lri = (LOGICAL_ALIAS_PAGE >> ROOT_INDEX_SHIFT) & (ROOT_TABLE_SIZE - 1UL);
    lpi = (LOGICAL_ALIAS_PAGE >> PTR_INDEX_SHIFT) & (PTR_TABLE_SIZE - 1UL);
    lti = (LOGICAL_ALIAS_PAGE >> PAGE_INDEX_SHIFT) & (PAGE_TABLE_SIZE - 1UL);
    alias_pte = (ULONG *)(pte_base + IDENTITY_PTRS * (PAGE_TABLE_SIZE * sizeof(ULONG)));
    ptr[lpi] = ((ULONG)alias_pte & 0xfffffff0UL) | TABLE_DESC;
    alias_pte[lti] = (tramp_page & 0xfffff000UL) | PAGE_DESC;

    ri = (tramp_page >> ROOT_INDEX_SHIFT) & (ROOT_TABLE_SIZE - 1UL);
    pi = (tramp_page >> PTR_INDEX_SHIFT) & (PTR_TABLE_SIZE - 1UL);
    ti = (tramp_page >> PAGE_INDEX_SHIFT) & (PAGE_TABLE_SIZE - 1UL);
    pte = (ULONG *)(pte_base + pi * (PAGE_TABLE_SIZE * sizeof(ULONG)));

    srp[0] = 0x80000002UL;
    srp[1] = (ULONG)root;

    Printf("table=0x%08lx tramp=0x%08lx identity_limit=0x%08lx\n",
           table_page, tramp_page, (ULONG)IDENTITY_LIMIT);
    Printf("identity ri=%lu pi=%lu ti=%lu root=%08lx ptr=%08lx pte=%08lx\n",
           ri, pi, ti, root[ri], ptr[pi], pte[ti]);
    Printf("alias=0x%08lx lri=%lu lpi=%lu lti=%lu ptr=%08lx pte=%08lx\n",
           (ULONG)LOGICAL_ALIAS_PAGE, lri, lpi, lti, ptr[lpi], alias_pte[lti]);

    marker("SYS:m1-3b4b6b-pmmu-armed.marker", "PMMU phased smoke armed\n");

    marker("SYS:m1-3b4b6b-pmmu-phase1-entered.marker", "identity phase entered\n");
    rc = run_phase(smoke, srp, 1UL);
    if (rc != 0) goto fail;
    marker("SYS:m1-3b4b6b-pmmu-identity-pass.marker", "identity enable/disable returned\n");
    Printf("M68KDEB_PMMU_PHASE1_PASS\n");

    marker("SYS:m1-3b4b6b-pmmu-phase2-entered.marker", "PLOADR phase entered\n");
    rc = run_phase(smoke, srp, 2UL);
    if (rc != 0) goto fail;
    marker("SYS:m1-3b4b6b-pmmu-pload-pass.marker", "PLOADR phase returned\n");
    Printf("M68KDEB_PMMU_PHASE2_PASS\n");

    marker("SYS:m1-3b4b6b-pmmu-phase3-entered.marker", "alias fetch phase entered\n");
    rc = run_phase(smoke, srp, 3UL);
    if (rc != 0) goto fail;

    Printf("M68KDEB_PMMU_SMOKE_RETURN rc=%ld\n", rc);
    marker("SYS:m1-3b4b6b-pmmu-returned.marker", "PMMU alias fetch returned\n");
    marker("SYS:m1-3b4b6b-pmmu-pass.marker", "PMMU phased alias fetch passed\n");
    Printf("M68KDEB_PMMU_SMOKE_PASS\n");
    goto out;

fail:
    marker("SYS:m1-3b4b6b-pmmu-fail.marker", "PMMU phase returned failure\n");
    Printf("M68KDEB_PMMU_SMOKE_FAIL rc=%ld\n", rc);

out:
    if (tramp_raw) FreeMem(tramp_raw, TRAMP_RAW_BYTES);
    if (table_raw) FreeMem(table_raw, TABLE_RAW_BYTES);
    return rc == 0 ? 0 : 20;
}
