/* M1.3b.4b.6b-PMMU isolated FS-UAE/AROS copied-code qualification. */
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

typedef LONG (*smoke_fn_t)(ULONG *unused_srp, ULONG unused_alias, UWORD *unused_scratch);

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
    LONG rc = 20;

    Printf("M68KDEB_PMMU_SMOKE_START\n");

    if (!m68kdeb_pmmu_smoke_blob_len || m68kdeb_pmmu_smoke_blob_len > PAGE_SIZE) {
        Printf("FAIL blob_len=%lu\n", (ULONG)m68kdeb_pmmu_smoke_blob_len);
        return 20;
    }

    tramp_raw = (UBYTE *)AllocMem(TRAMP_RAW_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    if (!tramp_raw) {
        Printf("FAIL alloc\n");
        return 20;
    }

    tramp_page = align_page((ULONG)tramp_raw);
    CopyMem(m68kdeb_pmmu_smoke_blob, (APTR)tramp_page,
            (ULONG)m68kdeb_pmmu_smoke_blob_len);
    CacheClearU();

    Printf("tramp=0x%08lx blob_len=%lu\n",
           tramp_page, (ULONG)m68kdeb_pmmu_smoke_blob_len);
    marker("SYS:m1-3b4b6b-pmmu-armed.marker",
           "68030 bare RTS copied-code control armed\n");

    rc = ((smoke_fn_t)tramp_page)(NULL, 0UL, NULL);

    Printf("M68KDEB_PMMU_SMOKE_RETURN rc=%ld\n", rc);
    if (rc == 0) {
        marker("SYS:m1-3b4b6b-pmmu-returned.marker",
               "68030 bare RTS copied-code control returned\n");
        marker("SYS:m1-3b4b6b-pmmu-pass.marker",
               "68030 bare RTS copied-code control passed\n");
        Printf("M68KDEB_PMMU_SMOKE_PASS\n");
    } else {
        marker("SYS:m1-3b4b6b-pmmu-fail.marker",
               "68030 bare RTS copied-code control failed\n");
        rc = 20;
    }

    FreeMem(tramp_raw, TRAMP_RAW_BYTES);
    return rc == 0 ? 0 : 20;
}
