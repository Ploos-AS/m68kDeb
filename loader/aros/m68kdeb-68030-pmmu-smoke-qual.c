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
#define LOGICAL_ALIAS_OFFSET 64UL

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

static void map_page(ULONG *root, ULONG *ptr, ULONG *pte,
                     ULONG logical, ULONG physical)
{
    ULONG ri = (logical >> ROOT_INDEX_SHIFT) & (ROOT_TABLE_SIZE - 1UL);
    ULONG pi = (logical >> PTR_INDEX_SHIFT) & (PTR_TABLE_SIZE - 1UL);
    ULONG ti = (logical >> PAGE_INDEX_SHIFT) & (PAGE_TABLE_SIZE - 1UL);

    root[ri] = ((ULONG)ptr & 0xffffff00UL) | TABLE_DESC;
    ptr[pi] = ((ULONG)pte & 0xffffff00UL) | TABLE_DESC;
    pte[ti] = (physical & 0xfffff000UL) | PAGE_DESC;
}

int main(void)
{
    UBYTE *table_raw = NULL, *tramp_raw = NULL;
    ULONG table_page, tramp_page, logical_alias_entry;
    ULONG *root, *ptr_phys, *ptr_log, *pte_phys, *pte_log;
    ULONG srp[2];
    ULONG pri, ppi, pti, lri, lpi, lti;
    APTR old_super;
    LONG pre_rc = 20, rc = 20;

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
    ptr_phys = (ULONG *)(table_page + 512UL);
    ptr_log = (ULONG *)(table_page + 1024UL);
    pte_phys = (ULONG *)(table_page + 1536UL);
    pte_log = (ULONG *)(table_page + 1792UL);

    CopyMem(m68kdeb_pmmu_smoke_blob, (APTR)tramp_page,
            (ULONG)m68kdeb_pmmu_smoke_blob_len);
    CacheClearU();

    map_page(root, ptr_phys, pte_phys, tramp_page, tramp_page);
    map_page(root, ptr_log, pte_log, LOGICAL_ALIAS_PAGE, tramp_page);

    pri = (tramp_page >> ROOT_INDEX_SHIFT) & (ROOT_TABLE_SIZE - 1UL);
    ppi = (tramp_page >> PTR_INDEX_SHIFT) & (PTR_TABLE_SIZE - 1UL);
    pti = (tramp_page >> PAGE_INDEX_SHIFT) & (PAGE_TABLE_SIZE - 1UL);
    lri = (LOGICAL_ALIAS_PAGE >> ROOT_INDEX_SHIFT) & (ROOT_TABLE_SIZE - 1UL);
    lpi = (LOGICAL_ALIAS_PAGE >> PTR_INDEX_SHIFT) & (PTR_TABLE_SIZE - 1UL);
    lti = (LOGICAL_ALIAS_PAGE >> PAGE_INDEX_SHIFT) & (PAGE_TABLE_SIZE - 1UL);

    srp[0] = 0x80000002UL;
    srp[1] = (ULONG)root;
    logical_alias_entry = LOGICAL_ALIAS_PAGE + LOGICAL_ALIAS_OFFSET;

    Printf("table=0x%08lx tramp=0x%08lx alias=0x%08lx\n",
           table_page, tramp_page, logical_alias_entry);
    Printf("physical ri=%lu pi=%lu ti=%lu root=%08lx ptr=%08lx pte=%08lx\n",
           pri, ppi, pti, root[pri], ptr_phys[ppi], pte_phys[pti]);
    Printf("logical ri=%lu pi=%lu ti=%lu root=%08lx ptr=%08lx pte=%08lx\n",
           lri, lpi, lti, root[lri], ptr_log[lpi], pte_log[lti]);
    Printf("srp=%08lx:%08lx tc=82c07760\n", srp[0], srp[1]);

    marker("SYS:m1-3b4b6b-pmmu-armed.marker", "PMMU dual-alias smoke armed\n");

    /* Stage A: same dual-alias tables, but do not take the alias jump. */
    Disable();
    old_super = SuperState();
    pre_rc = ((smoke_fn_t)tramp_page)(srp, logical_alias_entry, 0UL);
    if (old_super) UserState(old_super);
    Enable();

    Printf("M68KDEB_PMMU_PREALIAS_RETURN rc=%ld\n", pre_rc);
    if (pre_rc != 0) {
        marker("SYS:m1-3b4b6b-pmmu-prealias-fail.marker",
               "PMMU pre-alias stage failed\n");
        goto out;
    }
    marker("SYS:m1-3b4b6b-pmmu-prealias-pass.marker",
           "PMMU TC activation survived before alias jump\n");
    Printf("M68KDEB_PMMU_PREALIAS_PASS\n");

    /* Stage B: exact physical -> logical alias -> physical transition. */
    Disable();
    old_super = SuperState();
    rc = ((smoke_fn_t)tramp_page)(srp, logical_alias_entry, 1UL);
    if (old_super) UserState(old_super);
    Enable();

    Printf("M68KDEB_PMMU_SMOKE_RETURN rc=%ld\n", rc);
    if (rc == 0) {
        marker("SYS:m1-3b4b6b-pmmu-pass.marker", "PMMU dual-alias smoke passed\n");
        Printf("M68KDEB_PMMU_SMOKE_PASS\n");
    } else {
        marker("SYS:m1-3b4b6b-pmmu-fail.marker", "PMMU dual-alias smoke returned failure\n");
    }

out:
    if (tramp_raw) FreeMem(tramp_raw, TRAMP_RAW_BYTES);
    if (table_raw) FreeMem(table_raw, TABLE_RAW_BYTES);
    return rc == 0 ? 0 : 20;
}
