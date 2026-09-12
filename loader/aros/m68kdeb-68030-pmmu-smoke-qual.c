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
#define LOGICAL_ALIAS_OFFSET 96UL

extern unsigned char m68kdeb_pmmu_smoke_blob[];
extern unsigned int m68kdeb_pmmu_smoke_blob_len;

typedef LONG (*smoke_fn_t)(ULONG *srp, ULONG logical_alias_entry, ULONG mode);

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

static LONG run_stage(smoke_fn_t fn, ULONG *srp, ULONG logical_alias_entry)
{
    APTR old_super;
    LONG rc;

    Disable();
    old_super = SuperState();
    rc = fn(srp, logical_alias_entry, 0UL);
    if (old_super) UserState(old_super);
    Enable();
    return rc;
}

int main(void)
{
    UBYTE *table_raw = NULL, *tramp_raw = NULL, *dummy_raw = NULL;
    ULONG table_page, tramp_page, dummy_page, logical_alias_entry;
    ULONG *root, *ptr_phys, *ptr_log, *pte_phys, *pte_log;
    ULONG srp[2];
    ULONG pri, ppi, pti, lri, lpi, lti;
    smoke_fn_t fn;
    LONG rc = 20;

    Printf("M68KDEB_PMMU_SMOKE_START\n");

    if (!m68kdeb_pmmu_smoke_blob_len || m68kdeb_pmmu_smoke_blob_len > PAGE_SIZE) {
        Printf("FAIL blob_len=%lu\n", (ULONG)m68kdeb_pmmu_smoke_blob_len);
        return 20;
    }

    table_raw = (UBYTE *)AllocMem(TABLE_RAW_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    tramp_raw = (UBYTE *)AllocMem(TRAMP_RAW_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    dummy_raw = (UBYTE *)AllocMem(TRAMP_RAW_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    if (!table_raw || !tramp_raw || !dummy_raw) {
        Printf("FAIL alloc\n");
        goto out;
    }

    table_page = align_page((ULONG)table_raw);
    tramp_page = align_page((ULONG)tramp_raw);
    dummy_page = align_page((ULONG)dummy_raw);

    /* Preserve the original known-PASS physical table layout exactly. */
    root = (ULONG *)table_page;
    ptr_phys = (ULONG *)(table_page + 512UL);
    pte_phys = (ULONG *)(table_page + 1024UL);

    /* Extra low-logical chain lives entirely outside the known-good chain. */
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

    /* Known-good identity chain only. */
    root[pri] = ((ULONG)ptr_phys & 0xffffff00UL) | TABLE_DESC;
    ptr_phys[ppi] = ((ULONG)pte_phys & 0xffffff00UL) | TABLE_DESC;
    pte_phys[pti] = (tramp_page & 0xfffff000UL) | PAGE_DESC;

    srp[0] = 0x80000002UL;
    srp[1] = (ULONG)root;
    logical_alias_entry = LOGICAL_ALIAS_PAGE + LOGICAL_ALIAS_OFFSET;
    fn = (smoke_fn_t)tramp_page;

    Printf("table=0x%08lx tramp=0x%08lx dummy=0x%08lx alias=0x%08lx\n",
           table_page, tramp_page, dummy_page, logical_alias_entry);
    Printf("physical ri=%lu pi=%lu ti=%lu root=%08lx ptr=%08lx pte=%08lx\n",
           pri, ppi, pti, root[pri], ptr_phys[ppi], pte_phys[pti]);
    Printf("logical ri=%lu pi=%lu ti=%lu ptr_log=%08lx pte_log=%08lx\n",
           lri, lpi, lti, (ULONG)ptr_log, (ULONG)pte_log);
    Printf("srp=%08lx:%08lx tc=82c07760 descriptor_bisect=1\n",
           srp[0], srp[1]);

    marker("SYS:m1-3b4b6b-pmmu-armed.marker",
           "PMMU descriptor-chain bisect armed\n");

    /* Stage 0: exact known-PASS identity mapping, no logical descriptor. */
    rc = run_stage(fn, srp, logical_alias_entry);
    if (rc != 0) goto out;
    marker("SYS:m1-3b4b6b-pmmu-identity-pass.marker",
           "identity-only TC activation passed\n");

    /* Stage 1: add only the second root descriptor. Lower tables remain zero. */
    root[lri] = ((ULONG)ptr_log & 0xffffff00UL) | TABLE_DESC;
    CacheClearU();
    rc = run_stage(fn, srp, logical_alias_entry);
    if (rc != 0) goto out;
    marker("SYS:m1-3b4b6b-pmmu-root-pass.marker",
           "second root descriptor passed\n");

    /* Stage 2: add the pointer descriptor, while its page entry remains zero. */
    ptr_log[lpi] = ((ULONG)pte_log & 0xffffff00UL) | TABLE_DESC;
    CacheClearU();
    rc = run_stage(fn, srp, logical_alias_entry);
    if (rc != 0) goto out;
    marker("SYS:m1-3b4b6b-pmmu-pointer-pass.marker",
           "second root+pointer descriptors passed\n");

    /* Stage 3: complete the unused low alias to a distinct physical page. */
    pte_log[lti] = (dummy_page & 0xfffff000UL) | PAGE_DESC;
    CacheClearU();
    rc = run_stage(fn, srp, logical_alias_entry);
    if (rc != 0) goto out;
    marker("SYS:m1-3b4b6b-pmmu-fullchain-pass.marker",
           "full second descriptor chain passed\n");

    marker("SYS:m1-3b4b6b-pmmu-prealias-pass.marker",
           "all descriptor-chain bisect stages passed\n");
    marker("SYS:m1-3b4b6b-pmmu-returned.marker",
           "PMMU descriptor-chain bisect returned\n");
    marker("SYS:m1-3b4b6b-pmmu-pass.marker",
           "PMMU descriptor-chain bisect passed\n");
    Printf("M68KDEB_PMMU_PREALIAS_PASS\n");
    Printf("M68KDEB_PMMU_SMOKE_PASS\n");
    rc = 0;

out:
    if (dummy_raw) FreeMem(dummy_raw, TRAMP_RAW_BYTES);
    if (tramp_raw) FreeMem(tramp_raw, TRAMP_RAW_BYTES);
    if (table_raw) FreeMem(table_raw, TABLE_RAW_BYTES);
    return rc == 0 ? 0 : 20;
}
