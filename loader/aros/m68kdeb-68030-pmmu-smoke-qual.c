/* M1.3b.4b.6b-PMMU isolated FS-UAE/AROS Linux-style dual-alias page-table SRP/TC qualification. */
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>

#define PAGE_SIZE 4096UL
#define TABLE_RAW_BYTES (PAGE_SIZE * 3UL - 1UL)
#define TRAMP_RAW_BYTES (PAGE_SIZE * 3UL - 1UL)
#define ROOT_TABLE_SIZE 128UL
#define PTR_TABLE_SIZE 128UL
#define PAGE_TABLE_SIZE 64UL
#define ROOT_INDEX_SHIFT 25UL
#define PTR_INDEX_SHIFT 18UL
#define PAGE_INDEX_SHIFT 12UL
#define TABLE_DESC 0x0000000aUL
#define PAGE_DESC 0x00000019UL

extern unsigned char m68kdeb_pmmu_smoke_blob[];
extern unsigned int m68kdeb_pmmu_smoke_blob_len;

typedef LONG (*smoke_fn_t)(ULONG *srp, ULONG unused_alias, UWORD *stage);

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
    ULONG table_page, tramp_page;
    ULONG *root, *ptr, *pte, *ptr_alias, *pte_alias;
    ULONG srp[2];
    ULONG ri, pi, ti, alias_ri, alias_pi, alias_ti, alias_addr;
    UWORD *scratch;
    APTR old_user_sp;
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
    root = (ULONG *)table_page;
    ptr = (ULONG *)(table_page + 512UL);
    pte = (ULONG *)(table_page + 1024UL);\n    ptr_alias = (ULONG *)(table_page + 1536UL);\n    pte_alias = (ULONG *)(table_page + 2048UL);

    CopyMem(m68kdeb_pmmu_smoke_blob, (APTR)tramp_page,
            (ULONG)m68kdeb_pmmu_smoke_blob_len);
    *scratch = 0xffffU;

    /* TC is enabled briefly, so use the same real identity hierarchy as the
     * known-good PMMU control baseline. The trampoline itself performs no
     * PTESTR, alias access, or TT0 load. The trampoline performs one same-page data read through the identity mapping
     * while TC is active; the scratch sentinel remains untouched. */
    ri = (tramp_page >> ROOT_INDEX_SHIFT) & (ROOT_TABLE_SIZE - 1UL);
    pi = (tramp_page >> PTR_INDEX_SHIFT) & (PTR_TABLE_SIZE - 1UL);
    ti = (tramp_page >> PAGE_INDEX_SHIFT) & (PAGE_TABLE_SIZE - 1UL);
    root[ri] = ((ULONG)ptr & 0xffffff00UL) | TABLE_DESC;
    ptr[pi] = ((ULONG)pte & 0xffffff00UL) | TABLE_DESC;
    pte[ti] = (tramp_page & 0xfffff000UL) | PAGE_DESC;
    /* 6b.17: reproduce the sparse Linux fetch-page pattern observed by 6b.15:
     * only the executing page and its immediate successor are present while
     * neighboring PTEs remain zero. Keep the exact Linux TC/SRP geometry. */
    pte[ti] = (tramp_page & 0xfffff000UL) | PAGE_DESC;
    pte[(ti + 1UL) & (PAGE_TABLE_SIZE - 1UL)] =
        ((tramp_page + PAGE_SIZE) & 0xfffff000UL) | PAGE_DESC;

    /* 6b.17: add a second Linux-like root/pointer path that converges on the
     * same physical trampoline page. 0x40000000 selects a distinct 7-bit root
     * index while preserving the lower pointer/page indices. */
    alias_addr = tramp_page ^ 0x40000000UL;
    alias_ri = (alias_addr >> ROOT_INDEX_SHIFT) & (ROOT_TABLE_SIZE - 1UL);
    alias_pi = (alias_addr >> PTR_INDEX_SHIFT) & (PTR_TABLE_SIZE - 1UL);
    alias_ti = (alias_addr >> PAGE_INDEX_SHIFT) & (PAGE_TABLE_SIZE - 1UL);
    root[alias_ri] = ((ULONG)ptr_alias & 0xffffff00UL) | TABLE_DESC;
    ptr_alias[alias_pi] = ((ULONG)pte_alias & 0xffffff00UL) | TABLE_DESC;
    pte_alias[alias_ti] = (tramp_page & 0xfffff000UL) | PAGE_DESC;
    pte_alias[(alias_ti + 1UL) & (PAGE_TABLE_SIZE - 1UL)] =
        ((tramp_page + PAGE_SIZE) & 0xfffff000UL) | PAGE_DESC;

    srp[0] = 0x80000002UL;
    srp[1] = (ULONG)root;
    CacheClearU();

    Printf("table=0x%08lx tramp=0x%08lx scratch=0x%08lx ri=%lu pi=%lu ti=%lu\n",
           table_page, tramp_page, (ULONG)scratch, ri, pi, ti);
    Printf("srp=%08lx:%08lx root=%08lx ptr=%08lx pte=%08lx blob_len=%lu\n",
           srp[0], srp[1], root[ri], ptr[pi], pte[ti],
           (ULONG)m68kdeb_pmmu_smoke_blob_len);
    marker("SYS:m1-3b4b6b-pmmu-armed.marker",
           "68030 Linux-style dual-alias page-table control armed\n");

    /* Match the historical known-good PMMU qualifier envelope exactly: keep
     * task switching/interrupt delivery out of the active-translation window. */
    Disable();
    old_user_sp = SuperState();
    rc = ((smoke_fn_t)tramp_page)(srp, 0UL, scratch);
    if (old_user_sp) UserState(old_user_sp);
    Enable();

    Printf("M68KDEB_PMMU_SMOKE_RETURN rc=%ld stage=%04lx\n",
           rc, (ULONG)*scratch);
    /* The same-page constant is checked by the trampoline; scratch stays untouched. */
    if (rc == 0 && *scratch == 0xffffU) {
        marker("SYS:m1-3b4b6b-pmmu-returned.marker",
               "68030 Linux-style dual-alias page-table control returned\n");
        marker("SYS:m1-3b4b6b-pmmu-pass.marker",
               "68030 Linux-style dual-alias page-table control passed\n");
        Printf("M68KDEB_PMMU_SMOKE_PASS\n");
    } else {
        marker("SYS:m1-3b4b6b-pmmu-fail.marker",
               "68030 Linux-style dual-alias page-table control failed\n");
        rc = 20;
    }

out:
    if (tramp_raw) FreeMem(tramp_raw, TRAMP_RAW_BYTES);
    if (table_raw) FreeMem(table_raw, TABLE_RAW_BYTES);
    return rc == 0 ? 0 : 20;
}
