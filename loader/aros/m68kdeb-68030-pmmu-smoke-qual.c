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
#define SAME_HIERARCHY_MASK 0xfffc0000UL

extern unsigned char m68kdeb_pmmu_smoke_blob[];
extern unsigned int m68kdeb_pmmu_smoke_blob_len;

typedef LONG (*smoke_fn_t)(ULONG *srp, ULONG alias, UWORD *mmusr_out);

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
    ULONG table_page, tramp_page, logical_alias;
    ULONG *root, *ptr, *pte;
    ULONG *alias_ptr_table, *alias_pte_table;
    ULONG alias_root_desc, alias_ptr_desc, alias_pte_desc;
    UWORD *mmusr_out;
    ULONG srp[2];
    ULONG ri, pi, ti, lri, lpi, lti;
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
    mmusr_out = (UWORD *)(tramp_page + PAGE_SIZE - sizeof(UWORD));

    root = (ULONG *)table_page;             /* +0x000, 512-byte aligned */
    ptr = (ULONG *)(table_page + 512UL);    /* +0x200, 512-byte aligned */
    pte = (ULONG *)(table_page + 1024UL);   /* +0x400, 256-byte aligned */

    CopyMem(m68kdeb_pmmu_smoke_blob, (APTR)tramp_page,
            (ULONG)m68kdeb_pmmu_smoke_blob_len);
    *mmusr_out = 0xffffU;
    CacheClearU();

    ri = (tramp_page >> ROOT_INDEX_SHIFT) & (ROOT_TABLE_SIZE - 1UL);
    pi = (tramp_page >> PTR_INDEX_SHIFT) & (PTR_TABLE_SIZE - 1UL);
    ti = (tramp_page >> PAGE_INDEX_SHIFT) & (PAGE_TABLE_SIZE - 1UL);

    /*
     * Keep TIA (bits 31..25) and TIB (bits 24..18) identical to the
     * trampoline mapping, but select a different TIC (bits 17..12).
     * This creates a genuine logical alias without introducing another
     * root or pointer descriptor/table into the PMMU walk.
     */
    lti = (ti + 1UL) & (PAGE_TABLE_SIZE - 1UL);
    logical_alias = (tramp_page & SAME_HIERARCHY_MASK) |
                    (lti << PAGE_INDEX_SHIFT);
    lri = (logical_alias >> ROOT_INDEX_SHIFT) & (ROOT_TABLE_SIZE - 1UL);
    lpi = (logical_alias >> PTR_INDEX_SHIFT) & (PTR_TABLE_SIZE - 1UL);
    lti = (logical_alias >> PAGE_INDEX_SHIFT) & (PAGE_TABLE_SIZE - 1UL);

    if (lri != ri || lpi != pi || lti == ti) {
        Printf("FAIL same-hierarchy alias ri=%lu/%lu pi=%lu/%lu ti=%lu/%lu\n",
               ri, lri, pi, lpi, ti, lti);
        marker("SYS:m1-3b4b6b-pmmu-alias-path-fail.marker",
               "Same-hierarchy alias derivation failed\n");
        goto out;
    }

    root[ri] = ((ULONG)ptr & TABLE_ADDR_MASK) | TABLE_DESC;
    ptr[pi] = ((ULONG)pte & TABLE_ADDR_MASK) | TABLE_DESC;
    pte[ti] = (tramp_page & PAGE_ADDR_MASK) | PAGE_DESC;
    pte[lti] = (tramp_page & PAGE_ADDR_MASK) | PAGE_DESC;
    alias_ptr_table = ptr;
    alias_pte_table = pte;

    CacheClearU();

    srp[0] = 0x80000002UL;
    srp[1] = (ULONG)root;

    alias_root_desc = root[lri];
    alias_ptr_desc = alias_ptr_table[lpi];
    alias_pte_desc = alias_pte_table[lti];

    Printf("table=0x%08lx tramp=0x%08lx ri=%lu pi=%lu ti=%lu\n",
           table_page, tramp_page, ri, pi, ti);
    Printf("srp=%08lx:%08lx root=%08lx ptr=%08lx pte=%08lx\n",
           srp[0], srp[1], root[ri], ptr[pi], pte[ti]);
    Printf("same_root_ptr_alias=0x%08lx lri=%lu lpi=%lu lti=%lu shared_root=%lu shared_ptr=%lu\n",
           logical_alias, lri, lpi, lti,
           (ULONG)(lri == ri), (ULONG)(lri == ri && lpi == pi));
    Printf("alias_path root=%08lx ptr=%08lx pte=%08lx ptr_base=%08lx pte_base=%08lx target=%08lx\n",
           alias_root_desc, alias_ptr_desc, alias_pte_desc,
           (ULONG)alias_ptr_table, (ULONG)alias_pte_table, tramp_page);

    if ((alias_root_desc & TABLE_ADDR_MASK) !=
            ((ULONG)alias_ptr_table & TABLE_ADDR_MASK) ||
        (alias_root_desc & 3UL) != 2UL) {
        Printf("FAIL alias_root_desc=%08lx expected_base=%08lx\n",
               alias_root_desc, (ULONG)alias_ptr_table);
        marker("SYS:m1-3b4b6b-pmmu-alias-path-fail.marker",
               "Alias root descriptor mismatch\n");
        goto out;
    }

    if ((alias_ptr_desc & TABLE_ADDR_MASK) !=
            ((ULONG)alias_pte_table & TABLE_ADDR_MASK) ||
        (alias_ptr_desc & 3UL) != 2UL) {
        Printf("FAIL alias_ptr_desc=%08lx expected_base=%08lx\n",
               alias_ptr_desc, (ULONG)alias_pte_table);
        marker("SYS:m1-3b4b6b-pmmu-alias-path-fail.marker",
               "Alias pointer descriptor mismatch\n");
        goto out;
    }

    if ((alias_pte_desc & PAGE_ADDR_MASK) !=
            (tramp_page & PAGE_ADDR_MASK) ||
        (alias_pte_desc & 3UL) != 1UL) {
        Printf("FAIL alias_pte_desc=%08lx expected_page=%08lx\n",
               alias_pte_desc, tramp_page);
        marker("SYS:m1-3b4b6b-pmmu-alias-path-fail.marker",
               "Alias page descriptor mismatch\n");
        goto out;
    }

    marker("SYS:m1-3b4b6b-pmmu-alias-path-pass.marker",
           "Same-root/same-pointer alias descriptor path preflight passed\n");
    marker("SYS:m1-3b4b6b-pmmu-armed.marker",
           "PMMU same-hierarchy alias PTESTR level-0 probe armed\n");

    Disable();
    old_super = SuperState();
    rc = ((smoke_fn_t)tramp_page)(srp, logical_alias, mmusr_out);
    if (old_super) UserState(old_super);
    Enable();

    Printf("M68KDEB_PMMU_SMOKE_RETURN rc=%ld mmusr=%04lx\n",
           rc, (ULONG)*mmusr_out);
    if (rc == 0) {
        marker("SYS:m1-3b4b6b-pmmu-ptest-pass.marker",
               "Same-hierarchy alias PTESTR level 0 completed\n");
        marker("SYS:m1-3b4b6b-pmmu-returned.marker", "PMMU smoke returned\n");
        marker("SYS:m1-3b4b6b-pmmu-pass.marker", "PMMU smoke passed\n");
        Printf("M68KDEB_PMMU_ALIAS_PTEST_PASS mmusr=%04lx\n",
               (ULONG)*mmusr_out);
        Printf("M68KDEB_PMMU_SMOKE_PASS\n");
    } else {
        marker("SYS:m1-3b4b6b-pmmu-fail.marker",
               "PMMU same-hierarchy alias PTESTR probe failed\n");
    }

out:
    if (tramp_raw) FreeMem(tramp_raw, TRAMP_RAW_BYTES);
    if (table_raw) FreeMem(table_raw, TABLE_RAW_BYTES);
    return rc == 0 ? 0 : 20;
}
