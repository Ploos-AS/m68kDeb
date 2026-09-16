/* M1.3b.4b.6b-PMMU isolated FS-UAE/AROS MC68030 trampoline qualification. */
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>

#define PAGE_SIZE 4096UL
#define TRAMP_RAW_BYTES (PAGE_SIZE * 2UL - 1UL)

extern unsigned char m68kdeb_pmmu_smoke_blob[];
extern unsigned int m68kdeb_pmmu_smoke_blob_len;

typedef LONG (*smoke_fn_t)(ULONG *unused_srp, ULONG unused_alias, UWORD *scratch);

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
    UBYTE *tramp_raw = NULL;
    ULONG tramp_page;
    UWORD *scratch;
    LONG rc = 20;

    Printf("M68KDEB_PMMU_SMOKE_START\n");

    if (!m68kdeb_pmmu_smoke_blob_len ||
        m68kdeb_pmmu_smoke_blob_len > PAGE_SIZE - sizeof(UWORD)) {
        Printf("FAIL blob_len=%lu\n", (ULONG)m68kdeb_pmmu_smoke_blob_len);
        return 20;
    }

    tramp_raw = (UBYTE *)AllocMem(TRAMP_RAW_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    if (!tramp_raw) {
        Printf("FAIL alloc\n");
        return 20;
    }

    tramp_page = align_page((ULONG)tramp_raw);
    scratch = (UWORD *)(tramp_page + PAGE_SIZE - sizeof(UWORD));

    CopyMem(m68kdeb_pmmu_smoke_blob, (APTR)tramp_page,
            (ULONG)m68kdeb_pmmu_smoke_blob_len);
    *scratch = 0xffffU;
    CacheClearU();

    Printf("tramp=0x%08lx blob_len=%lu\n",
           tramp_page, (ULONG)m68kdeb_pmmu_smoke_blob_len);
    marker("SYS:m1-3b4b6b-pmmu-armed.marker",
           "68030 direct trampoline ABI control armed\n");

    /*
     * Deliberately stay in the caller's normal execution mode here.  The
     * current trampoline only executes ordinary instructions plus MOVEC CACR;
     * this bisect establishes whether the copied-code ABI/stack/RTS path is
     * sound independently of SuperState()/UserState() and Disable()/Enable().
     */
    rc = ((smoke_fn_t)tramp_page)(NULL, 0UL, scratch);

    Printf("M68KDEB_PMMU_SMOKE_RETURN rc=%ld stage=%04lx\n",
           rc, (ULONG)*scratch);
    if (rc == 0 && *scratch == 0x1111U) {
        marker("SYS:m1-3b4b6b-pmmu-returned.marker",
               "68030 direct trampoline ABI control returned\n");
        marker("SYS:m1-3b4b6b-pmmu-pass.marker",
               "68030 direct trampoline ABI control passed\n");
        Printf("M68KDEB_PMMU_SMOKE_PASS\n");
        rc = 0;
    } else {
        marker("SYS:m1-3b4b6b-pmmu-fail.marker",
               "68030 direct trampoline ABI control failed\n");
        rc = 20;
    }

    FreeMem(tramp_raw, TRAMP_RAW_BYTES);
    return rc == 0 ? 0 : 20;
}
