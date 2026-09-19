/* M1.3b.4b.6b-PMMU isolated FS-UAE/AROS alias data-read FCL-off qualification. */
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

extern unsigned char m68kdeb_pmmu_smoke_blob[];
extern unsigned int m68kdeb_pmmu_smoke_blob_len;

typedef LONG (*smoke_fn_t)(ULONG *srp, ULONG alias_page, UWORD *stage);

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
    ULONG *root, *ptr, *pte;
    ULONG srp[2];
    ULONG ri, pi, ti, alias_page, ari, api, ati;
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
    pte = (ULONG *)(table_page + 1024UL);

    CopyMem(m68kdeb_pmmu_smoke_blob, (APTR)tramp_page,
            (ULONG)m68kdeb_pmmu_smoke_blob_len);
    *scratch = 0xffffU;
    alias_page = tramp_page ^ PAGE_SIZE;

    /* Build the known-good three-level identity hierarchy, plus a neighbouring
     * logical page that aliases the trampoline's physical page. */
    ri = (tramp_page >> ROOT_INDEX_SHIFT) & (ROOT_TABLE_SIZE - 1UL);
    pi = (tramp_page >> PTR_INDEX_SHIFT) & (PTR_TABLE_SIZE - 1UL);
    ti = (tramp_page >> PAGE_INDEX_SHIFT) & (PAGE_TABLE_SIZE - 1UL);
    root[ri] = ((ULONG)ptr & 0xffffff00UL) | TABLE_DESC;
    ptr[pi] = ((ULONG)pte & 0xffffff00UL) | TABLE_DESC;
    pte[ti] = (tramp_page & 0xfffff000UL) | PAGE_DESC;

    ari = (alias_page >> ROOT_INDEX_SHIFT) & (ROOT_TABLE_SIZE - 1UL);
    api = (alias_page >> PTR_INDEX_SHIFT) & (PTR_TABLE_SIZE - 1UL);
    ati = (alias_page >> PAGE_INDEX_SHIFT) & (PAGE_TABLE_SIZE - 1UL);
    if (ari != ri || api != pi || ati == ti) {
        Printf("FAIL alias geometry alias=0x%08lx\n", alias_page);
        goto out;
    }
    /* Control: keep the alias slot invalid. If this restores the known-good\n     * identity-read result, merely installing the duplicate mapping is causal. */\n    pte[ati] = 0;\n\n    srp[0] = 0x80000002UL;
    srp[1] = (ULONG)root;
    CacheClearU();

    Printf("table=0x%08lx tramp=0x%08lx alias=0x%08lx scratch=0x%08lx ri=%lu pi=%lu ti=%lu ati=%lu\n",
           table_page, tramp_page, alias_page, (ULONG)scratch, ri, pi, ti, ati);
    Printf("srp=%08lx:%08lx root=%08lx ptr=%08lx pte=%08lx alias_pte(control-invalid)=%08lx blob_len=%lu\n",
           srp[0], srp[1], root[ri], ptr[pi], pte[ti], pte[ati],
           (ULONG)m68kdeb_pmmu_smoke_blob_len);
    marker("SYS:m1-3b4b6b-pmmu-armed.marker",
           "68030 alias data-read FCL-off control armed\n");

    Disable();
    old_user_sp = SuperState();
    rc = ((smoke_fn_t)tramp_page)(srp, alias_page, scratch);
    if (old_user_sp) UserState(old_user_sp);
    Enable();

    Printf("M68KDEB_PMMU_SMOKE_RETURN rc=%ld stage=%04lx\n",
           rc, (ULONG)*scratch);
    if (rc == 0 && *scratch == 0xffffU) {
        marker("SYS:m1-3b4b6b-pmmu-returned.marker",
               "68030 alias data-read FCL-off control returned\n");
        marker("SYS:m1-3b4b6b-pmmu-pass.marker",
               "68030 alias data-read FCL-off control passed\n");
        Printf("M68KDEB_PMMU_SMOKE_PASS\n");
    } else {
        marker("SYS:m1-3b4b6b-pmmu-fail.marker",
               "68030 alias data-read FCL-off control failed\n");
        rc = 20;
    }

out:
    if (tramp_raw) FreeMem(tramp_raw, TRAMP_RAW_BYTES);
    if (table_raw) FreeMem(table_raw, TABLE_RAW_BYTES);
    return rc == 0 ? 0 : 20;
}
