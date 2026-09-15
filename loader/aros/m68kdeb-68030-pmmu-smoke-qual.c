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
#define TABLE_ADDR_MASK 0xffffff00UL
#define PAGE_ADDR_MASK 0xfffff000UL
#define LOGICAL_ALIAS 0x80000000UL

extern unsigned char m68kdeb_pmmu_smoke_blob[];
extern unsigned int m68kdeb_pmmu_smoke_blob_len;

typedef LONG (*smoke_fn_t)(ULONG *srp, ULONG alias, UWORD *scratch);

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

int main(void)
{
    UBYTE *table_raw = NULL, *tramp_raw = NULL;
    ULONG table_page, tramp_page, logical_alias = LOGICAL_ALIAS;
    ULONG *root, *alias_ptr, *alias_pte;
    ULONG alias_root_desc, alias_ptr_desc, alias_pte_desc;
    UWORD *scratch;
    ULONG srp[2];
    ULONG lri, lpi, lti;
    APTR old_super;
    LONG rc = 20;

    Printf("M68KDEB_PMMU_SMOKE_START\n");

    if (!m68kdeb_pmmu_smoke_blob_len ||
        m68kdeb_pmmu_smoke_blob_len > PAGE_SIZE - sizeof(UWORD)) {
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
    scratch = (UWORD *)(tramp_page + PAGE_SIZE - sizeof(UWORD));

    if ((table_page | tramp_page) & 0x80000000UL) {
        Printf("FAIL qualification memory outside TT0 lower-half window\n");
        goto out;
    }

    root = (ULONG *)table_page;
    alias_ptr = (ULONG *)(table_page + 512UL);
    alias_pte = (ULONG *)(table_page + 1024UL);

    CopyMem(m68kdeb_pmmu_smoke_blob, (APTR)tramp_page,
            (ULONG)m68kdeb_pmmu_smoke_blob_len);
    *scratch = 0xffffU;
    CacheClearU();

    lri = (logical_alias >> ROOT_INDEX_SHIFT) & (ROOT_TABLE_SIZE - 1UL);
    lpi = (logical_alias >> PTR_INDEX_SHIFT) & (PTR_TABLE_SIZE - 1UL);
    lti = (logical_alias >> PAGE_INDEX_SHIFT) & (PAGE_TABLE_SIZE - 1UL);

    root[lri] = ((ULONG)alias_ptr & TABLE_ADDR_MASK) | TABLE_DESC;
    alias_ptr[lpi] = ((ULONG)alias_pte & TABLE_ADDR_MASK) | TABLE_DESC;
    alias_pte[lti] = (tramp_page & PAGE_ADDR_MASK) | PAGE_DESC;
    CacheClearU();

    srp[0] = 0x80000002UL;
    srp[1] = (ULONG)root;
    alias_root_desc = root[lri];
    alias_ptr_desc = alias_ptr[lpi];
    alias_pte_desc = alias_pte[lti];

    Printf("table=0x%08lx tramp=0x%08lx alias=0x%08lx\n",
           table_page, tramp_page, logical_alias);
    Printf("alias_path ri=%lu pi=%lu ti=%lu root=%08lx ptr=%08lx pte=%08lx\n",
           lri, lpi, lti, alias_root_desc, alias_ptr_desc, alias_pte_desc);
    Printf("tt0_lower_2g_identity=1 target=%08lx\n", tramp_page);

    if ((alias_root_desc & TABLE_ADDR_MASK) != ((ULONG)alias_ptr & TABLE_ADDR_MASK) ||
        (alias_root_desc & 3UL) != 2UL ||
        (alias_ptr_desc & TABLE_ADDR_MASK) != ((ULONG)alias_pte & TABLE_ADDR_MASK) ||
        (alias_ptr_desc & 3UL) != 2UL ||
        (alias_pte_desc & PAGE_ADDR_MASK) != (tramp_page & PAGE_ADDR_MASK) ||
        (alias_pte_desc & 3UL) != 1UL) {
        Printf("FAIL alias descriptor path\n");
        marker("SYS:m1-3b4b6b-pmmu-alias-path-fail.marker",
               "Alias descriptor path mismatch\n");
        goto out;
    }

    marker("SYS:m1-3b4b6b-pmmu-alias-path-pass.marker",
           "High-half alias descriptor path preflight passed\n");
    marker("SYS:m1-3b4b6b-pmmu-armed.marker",
           "PMMU TT0/SRP/TC activation bisect armed\n");

    Disable();
    old_super = SuperState();
    rc = ((smoke_fn_t)tramp_page)(srp, logical_alias, scratch);
    if (old_super) UserState(old_super);
    Enable();

    Printf("M68KDEB_PMMU_SMOKE_RETURN rc=%ld stage=%04lx\n",
           rc, (ULONG)*scratch);
    if (rc == 0 && *scratch == 0x1111U) {
        marker("SYS:m1-3b4b6b-pmmu-returned.marker",
               "PMMU TT0/SRP/TC activation returned\n");
        marker("SYS:m1-3b4b6b-pmmu-pass.marker",
               "PMMU TT0/SRP/TC activation bisect passed\n");
        Printf("M68KDEB_PMMU_ALIAS_FETCH_PASS\n");
        Printf("M68KDEB_PMMU_SMOKE_PASS\n");
    } else {
        rc = 20;
        marker("SYS:m1-3b4b6b-pmmu-fail.marker",
               "PMMU TT0/SRP/TC activation bisect failed\n");
    }

out:
    if (tramp_raw) FreeMem(tramp_raw, TRAMP_RAW_BYTES);
    if (table_raw) FreeMem(table_raw, TABLE_RAW_BYTES);
    return rc == 0 ? 0 : 20;
}
