#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <stdio.h>
#include <stdint.h>

#include "m68kdeb-68030-mmu-state.h"
#include "m68kdeb-68030-identity-window.h"

#define PAGE_SIZE 4096UL
#define RAW_BYTES (PAGE_SIZE * 2UL - 1UL)

extern unsigned char m68kdeb_trampoline_blob[];
extern unsigned int m68kdeb_trampoline_blob_len;
extern unsigned int m68kdeb_trampoline_entry_offset;
extern unsigned int m68kdeb_trampoline_mmu_off_offset;
extern unsigned int m68kdeb_trampoline_post_mmu_offset;
extern unsigned int m68kdeb_trampoline_tc_zero_offset;

static struct MemHeader *find_owner(struct ExecBase *SysBase, ULONG base, ULONG bytes)
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

static int inside(ULONG off, ULONG bytes)
{
    return off < bytes;
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
    ULONG entry = (ULONG)m68kdeb_trampoline_entry_offset;
    ULONG mmu = (ULONG)m68kdeb_trampoline_mmu_off_offset;
    ULONG post = (ULONG)m68kdeb_trampoline_post_mmu_offset;
    ULONG zero = (ULONG)m68kdeb_trampoline_tc_zero_offset;
    int rc;

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("M68KDEB_5T_START\n");
    printf("blob_bytes=%lu entry_offset=%lu mmu_offset=%lu post_offset=%lu tc_zero_offset=%lu\n",
           blob_len, entry, mmu, post, zero);

    if (!blob_len || blob_len > PAGE_SIZE || !inside(entry, blob_len) ||
        !inside(mmu, blob_len) || !inside(post, blob_len) ||
        zero > blob_len - 4UL || !(entry < mmu && mmu < post && post < zero)) {
        printf("M68KDEB_5T_BAD_LAYOUT\n");
        return 9;
    }

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

    if ((ULONG)proof.logical_base + entry != (ULONG)proof.physical_base + entry ||
        (ULONG)proof.logical_base + mmu != (ULONG)proof.physical_base + mmu ||
        (ULONG)proof.logical_base + post != (ULONG)proof.physical_base + post ||
        (ULONG)proof.logical_base + zero != (ULONG)proof.physical_base + zero) {
        printf("M68KDEB_5T_ADDRESS_BIND_FAIL\n");
        FreeMem(raw, RAW_BYTES);
        return 22;
    }

    if (page[zero] || page[zero + 1] || page[zero + 2] || page[zero + 3]) {
        printf("M68KDEB_5T_TC_ZERO_FAIL\n");
        FreeMem(raw, RAW_BYTES);
        return 23;
    }

    printf("entry_logical=0x%08lx entry_physical=0x%08lx\n",
           (ULONG)proof.logical_base + entry, (ULONG)proof.physical_base + entry);
    printf("mmu_off_logical=0x%08lx mmu_off_physical=0x%08lx\n",
           (ULONG)proof.logical_base + mmu, (ULONG)proof.physical_base + mmu);
    printf("post_mmu_logical=0x%08lx post_mmu_physical=0x%08lx\n",
           (ULONG)proof.logical_base + post, (ULONG)proof.physical_base + post);
    printf("tc_zero_logical=0x%08lx tc_zero_physical=0x%08lx\n",
           (ULONG)proof.logical_base + zero, (ULONG)proof.physical_base + zero);
    printf("M68KDEB_5T_TRANSITION_LAYOUT_BOUND\n");
    printf("transition_entry_execute=NOT_ATTEMPTED\n");
    printf("mmu_mutation=NOT_ATTEMPTED\n");
    printf("cache_mutation=NOT_ATTEMPTED\n");
    printf("linux_jump=NOT_ATTEMPTED\n");
    printf("M68KDEB_5T_RETURNED\n");

    FreeMem(raw, RAW_BYTES);
    return 0;
}
