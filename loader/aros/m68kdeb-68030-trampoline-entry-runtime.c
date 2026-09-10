#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <stdio.h>
#include <stdint.h>

#include "m68kdeb-68030-mmu-state.h"
#include "m68kdeb-68030-identity-window.h"

#define PAGE_SIZE 4096UL
#define RAW_BYTES (PAGE_SIZE * 2UL - 1UL)

typedef int (*m68kdeb_probe_fn)(void);

extern unsigned char m68kdeb_trampoline_blob[];
extern unsigned int m68kdeb_trampoline_blob_len;
extern unsigned int m68kdeb_trampoline_reversible_probe_offset;

static struct MemHeader *find_owner(struct ExecBase *SysBase,
                                    ULONG base, ULONG bytes)
{
    struct MemHeader *mh;
    ULONG end;

    if (!bytes || base > 0xffffffffUL - bytes)
        return NULL;
    end = base + bytes;
    for (mh = (struct MemHeader *)SysBase->MemList.lh_Head;
         mh->mh_Node.ln_Succ != NULL;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        ULONG lower = (ULONG)mh->mh_Lower;
        ULONG upper = (ULONG)mh->mh_Upper;
        if (base >= lower && end <= upper)
            return mh;
    }
    return NULL;
}

static int bytes_equal(const UBYTE *a, const UBYTE *b, ULONG n)
{
    ULONG i;
    for (i = 0; i < n; i++)
        if (a[i] != b[i])
            return 0;
    return 1;
}

int main(void)
{
    struct ExecBase *SysBase = *(struct ExecBase **)4;
    UBYTE *raw, *page;
    struct MemHeader *owner;
    struct m68kdeb_physwin_candidate proof;
    APTR oldsp;
    unsigned long tc = 0;
    ULONG blob_len = (ULONG)m68kdeb_trampoline_blob_len;
    ULONG probe_offset = (ULONG)m68kdeb_trampoline_reversible_probe_offset;
    m68kdeb_probe_fn probe;
    int rc;

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("M68KDEB_5S_START\n");
    printf("blob_bytes=%lu probe_offset=%lu\n", blob_len, probe_offset);

    if (!blob_len || blob_len > PAGE_SIZE || probe_offset >= blob_len)
        return 9;

    raw = (UBYTE *)AllocMem(RAW_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    if (!raw) return 10;
    page = (UBYTE *)(((ULONG)raw + PAGE_SIZE - 1UL) & ~(PAGE_SIZE - 1UL));
    owner = find_owner(SysBase, (ULONG)page, PAGE_SIZE);
    if (!owner) {
        FreeMem(raw, RAW_BYTES);
        return 11;
    }

    oldsp = SuperState();
    rc = m68kdeb_68030_read_tc(&tc);
    if (oldsp) UserState(oldsp);
    printf("tc=0x%08lx tc_rc=%ld\n", tc, (long)rc);
    if (rc != 0) {
        FreeMem(raw, RAW_BYTES);
        return 12;
    }

    rc = m68kdeb_68030_identity_window_from_tc(
        (uint32_t)tc, (uint32_t)(ULONG)page, (uint32_t)PAGE_SIZE,
        (uint32_t)blob_len, (uint32_t)PAGE_SIZE,
        (uint32_t)(ULONG)owner->mh_Lower,
        (uint32_t)((ULONG)owner->mh_Upper - (ULONG)owner->mh_Lower), &proof);
    printf("identity_rc=%ld\n", (long)rc);
    if (rc != M68KDEB_68030_IDWIN_OK) {
        FreeMem(raw, RAW_BYTES);
        return 20;
    }

    CopyMem((APTR)m68kdeb_trampoline_blob, (APTR)page, blob_len);
    if (!bytes_equal(page, m68kdeb_trampoline_blob, blob_len)) {
        FreeMem(raw, RAW_BYTES);
        return 21;
    }
    printf("M68KDEB_5S_COPY_VERIFIED\n");

    probe = (m68kdeb_probe_fn)(page + probe_offset);
    printf("M68KDEB_5S_BEFORE_PROBE\n");
    rc = probe();
    printf("M68KDEB_5S_AFTER_PROBE rc=%ld\n", (long)rc);
    if (rc != 0) {
        FreeMem(raw, RAW_BYTES);
        return 22;
    }

    printf("logical_base=0x%08lx\n", (ULONG)proof.logical_base);
    printf("physical_base=0x%08lx\n", (ULONG)proof.physical_base);
    printf("M68KDEB_5S_COPIED_PROBE_RETURNED\n");
    printf("transition_entry=NOT_ATTEMPTED\n");
    printf("mmu_mutation=NOT_ATTEMPTED\n");
    printf("cache_mutation=NOT_ATTEMPTED\n");
    printf("linux_jump=NOT_ATTEMPTED\n");
    printf("M68KDEB_5S_RETURNED\n");

    FreeMem(raw, RAW_BYTES);
    return 0;
}
