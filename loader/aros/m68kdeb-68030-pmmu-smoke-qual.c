/* M1.3b.4b.6b-PMMU isolated FS-UAE/AROS MC68030 PMMU qualification. */
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>

#define PAGE_SIZE 4096UL
#define TABLE_RAW_BYTES (PAGE_SIZE * 2UL - 1UL)
#define TRAMP_RAW_BYTES (PAGE_SIZE * 2UL - 1UL)
#define ROOT_TABLE_SIZE 128UL
#define PTR_TABLE_SIZE 128UL
#define PAGE_TABLE_SIZE 64UL
#define ROOT_INDEX_SHIFT 25UL
#define PTR_INDEX_SHIFT 18UL
#define PAGE_INDEX_SHIFT 12UL
#define TABLE_DESC 0x0000000aUL
#define PAGE_DESC 0x00000019UL
#define LOGICAL_ALIAS_PAGE 0x005db000UL

extern unsigned char m68kdeb_pmmu_smoke_blob[];
extern unsigned int m68kdeb_pmmu_smoke_blob_len;

typedef LONG (*smoke_fn_t)(ULONG *srp);

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

static LONG run_stage(smoke_fn_t fn, ULONG *srp)
{
    APTR old_super;
    LONG rc;
    Disable();
    old_super = SuperState();
    rc = fn(srp);
    if (old_super) UserState(old_super);
    Enable();
    return rc;
}

int main(void)
{
    UBYTE *table_raw = NULL, *tramp_raw = NULL, *dummy_raw = NULL;
    ULONG table_page, tramp_page, dummy_page;
    ULONG *root, *ptr_phys, *ptr_log, *pte_phys, *pte_log;
    ULONG srp[2];
    ULONG pri, ppi, pti, lri, lpi, lti;
    smoke_fn_t fn;
    LONG rc = 20;

    Printf("M68KDEB_PMMU_SMOKE_START\n");
    if (!m68kdeb_pmmu_smoke_blob_len || m68kdeb_pmmu_smoke_blob_len > PAGE_SIZE)
        return 20;

    table_raw = (UBYTE *)AllocMem(TABLE_RAW_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    tramp_raw = (UBYTE *)AllocMem(TRAMP_RAW_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    dummy_raw = (UBYTE *)AllocMem(TRAMP_RAW_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    if (!table_raw || !tramp_raw || !dummy_raw) goto out;

    table_page = align_page((ULONG)table_raw);
    tramp_page = align_page((ULONG)tramp_raw);
    dummy_page = align_page((ULONG)dummy_raw);

    /* Keep the original known-PASS physical chain at exactly +0/+512/+1024. */
    root = (ULONG *)table_page;
    ptr_phys = (ULONG *)(table_page + 512UL);
    pte_phys = (ULONG *)(table_page + 1024UL);
    ptr_log = (ULONG *)(table_page + 1280UL);
    pte_log = (ULONG *)(table_page + 1792UL);

    CopyMem(m68kdeb_pmmu_smoke_blob, (APTR)tramp_page,
            (ULONG)m68kdeb_pmmu_smoke_blob_len);
    CacheClearU();

    pri = (tramp_page >> ROOT_INDEX_SHIFT) & (ROOT_TABLE_SIZE - 1UL);
    ppi = (tramp_page >> PTR_INDEX_SHIFT) & (PTR_TABLE_SIZE - 1UL);
    pti = (tramp_page >> PAGE_INDEX_SHIFT) & (PAGE_TABLE_SIZE - 1UL);
    lri = (LOGICAL_ALIAS_PAGE >> ROOT_INDEX_SHIFT) & (ROOT_TABLE_SIZE - 1UL);
    lpi = (LOGICAL_ALIAS_PAGE >> PTR_INDEX_SHIFT) & (PTR_TABLE_SIZE - 1UL);
    lti = (LOGICAL_ALIAS_PAGE >> PAGE_INDEX_SHIFT) & (PAGE_TABLE_SIZE - 1UL);

    root[pri] = ((ULONG)ptr_phys & 0xffffff00UL) | TABLE_DESC;
    ptr_phys[ppi] = ((ULONG)pte_phys & 0xffffff00UL) | TABLE_DESC;
    pte_phys[pti] = (tramp_page & 0xfffff000UL) | PAGE_DESC;

    srp[0] = 0x80000002UL;
    srp[1] = (ULONG)root;
    fn = (smoke_fn_t)tramp_page;

    Printf("table=%08lx tramp=%08lx dummy=%08lx pri=%lu ppi=%lu pti=%lu lri=%lu lpi=%lu lti=%lu\n",
           table_page, tramp_page, dummy_page, pri, ppi, pti, lri, lpi, lti);
    marker("SYS:m1-3b4b6b-pmmu-armed.marker", "exact-trampoline descriptor bisect armed\n");

    /* Stage 0 must reproduce the original known-PASS experiment. */
    rc = run_stage(fn, srp);
    if (rc != 0) goto out;
    marker("SYS:m1-3b4b6b-pmmu-identity-pass.marker", "identity-only passed\n");

    /* Add one descriptor level at a time, never executing through this alias. */
    root[lri] = ((ULONG)ptr_log & 0xffffff00UL) | TABLE_DESC;
    CacheClearU();
    rc = run_stage(fn, srp);
    if (rc != 0) goto out;
    marker("SYS:m1-3b4b6b-pmmu-root-pass.marker", "second root passed\n");

    ptr_log[lpi] = ((ULONG)pte_log & 0xffffff00UL) | TABLE_DESC;
    CacheClearU();
    rc = run_stage(fn, srp);
    if (rc != 0) goto out;
    marker("SYS:m1-3b4b6b-pmmu-pointer-pass.marker", "second pointer passed\n");

    pte_log[lti] = (dummy_page & 0xfffff000UL) | PAGE_DESC;
    CacheClearU();
    rc = run_stage(fn, srp);
    if (rc != 0) goto out;
    marker("SYS:m1-3b4b6b-pmmu-fullchain-pass.marker", "full second chain passed\n");
    marker("SYS:m1-3b4b6b-pmmu-prealias-pass.marker", "descriptor bisect passed\n");
    marker("SYS:m1-3b4b6b-pmmu-returned.marker", "descriptor bisect returned\n");
    marker("SYS:m1-3b4b6b-pmmu-pass.marker", "PMMU smoke passed\n");
    Printf("M68KDEB_PMMU_PREALIAS_PASS\nM68KDEB_PMMU_SMOKE_PASS\n");
    rc = 0;

out:
    if (dummy_raw) FreeMem(dummy_raw, TRAMP_RAW_BYTES);
    if (tramp_raw) FreeMem(tramp_raw, TRAMP_RAW_BYTES);
    if (table_raw) FreeMem(table_raw, TABLE_RAW_BYTES);
    return rc == 0 ? 0 : 20;
}
