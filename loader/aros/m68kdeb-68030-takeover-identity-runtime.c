#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <stdio.h>
#include <stdint.h>

#include "m68kdeb-68030-mmu-state.h"
#include "m68kdeb-68030-takeover-identity.h"

#define PAGE_SIZE 4096UL
#define LAYOUT_RAW_BYTES (PAGE_SIZE * 3UL - 1UL)
#define INIT_RAW_BYTES (PAGE_SIZE * 2UL - 1UL)

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

int main(void)
{
    struct ExecBase *SysBase = *(struct ExecBase **)4;
    UBYTE *layout_raw, *layout_base, *init_raw, *init_base;
    struct MemHeader *layout_owner, *init_owner;
    struct m68kdeb_takeover_request req;
    struct m68kdeb_68030_takeover_identity_proof proof;
    APTR oldsp;
    unsigned long tc = 0;
    int preflight_rc, identity_rc, tc_rc;

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("M68KDEB_5U_START\n");

    layout_raw = (UBYTE *)AllocMem(LAYOUT_RAW_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    init_raw = (UBYTE *)AllocMem(INIT_RAW_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    if (!layout_raw || !init_raw) {
        printf("M68KDEB_5U_ALLOC_FAIL\n");
        if (layout_raw) FreeMem(layout_raw, LAYOUT_RAW_BYTES);
        if (init_raw) FreeMem(init_raw, INIT_RAW_BYTES);
        return 10;
    }

    layout_base = (UBYTE *)(((ULONG)layout_raw + PAGE_SIZE - 1UL) & ~(PAGE_SIZE - 1UL));
    init_base = (UBYTE *)(((ULONG)init_raw + PAGE_SIZE - 1UL) & ~(PAGE_SIZE - 1UL));

    req.layout_base = (ULONG)layout_base;
    req.layout_bytes = PAGE_SIZE;
    req.entry_addr = (ULONG)layout_base + 64UL;
    req.bootinfo_addr = (ULONG)layout_base + PAGE_SIZE;
    req.bootinfo_bytes = 256UL;
    req.initramfs_addr = (ULONG)init_base;
    req.initramfs_bytes = PAGE_SIZE;
    req.cpu = M68KDEB_CPU_68030;
    req.mmu = M68KDEB_MMU_68030;

    layout_owner = find_owner(SysBase, req.layout_base,
                              req.layout_bytes + req.bootinfo_bytes);
    init_owner = find_owner(SysBase, req.initramfs_addr, req.initramfs_bytes);
    if (!layout_owner || !init_owner) {
        printf("M68KDEB_5U_OWNER_FAIL\n");
        FreeMem(init_raw, INIT_RAW_BYTES);
        FreeMem(layout_raw, LAYOUT_RAW_BYTES);
        return 11;
    }

    preflight_rc = m68kdeb_takeover_68030_prepare(&req);
    printf("preflight_rc=%ld\n", (long)preflight_rc);
    if (preflight_rc != M68KDEB_TAKEOVER_OK) {
        FreeMem(init_raw, INIT_RAW_BYTES);
        FreeMem(layout_raw, LAYOUT_RAW_BYTES);
        return 12;
    }

    oldsp = SuperState();
    tc_rc = m68kdeb_68030_read_tc(&tc);
    if (oldsp) UserState(oldsp);
    printf("tc=0x%08lx tc_rc=%ld\n", tc, (long)tc_rc);
    if (tc_rc != 0) {
        FreeMem(init_raw, INIT_RAW_BYTES);
        FreeMem(layout_raw, LAYOUT_RAW_BYTES);
        return 13;
    }

    identity_rc = m68kdeb_68030_takeover_identity_from_tc(
        (uint32_t)tc, &req,
        (uint32_t)(ULONG)layout_owner->mh_Lower,
        (uint32_t)((ULONG)layout_owner->mh_Upper - (ULONG)layout_owner->mh_Lower),
        (uint32_t)(ULONG)init_owner->mh_Lower,
        (uint32_t)((ULONG)init_owner->mh_Upper - (ULONG)init_owner->mh_Lower),
        &proof);
    printf("identity_rc=%ld\n", (long)identity_rc);
    if (identity_rc != M68KDEB_68030_TAKEID_OK) {
        FreeMem(init_raw, INIT_RAW_BYTES);
        FreeMem(layout_raw, LAYOUT_RAW_BYTES);
        return 20;
    }

    printf("layout_logical=0x%08lx layout_physical=0x%08lx bytes=%lu\n",
           (ULONG)proof.layout_logical, (ULONG)proof.layout_physical,
           (ULONG)proof.layout_plus_bootinfo_bytes);
    printf("entry_logical=0x%08lx entry_physical=0x%08lx\n",
           (ULONG)proof.entry_logical, (ULONG)proof.entry_physical);
    printf("bootinfo_logical=0x%08lx bootinfo_physical=0x%08lx\n",
           (ULONG)proof.bootinfo_logical, (ULONG)proof.bootinfo_physical);
    printf("initramfs_logical=0x%08lx initramfs_physical=0x%08lx bytes=%lu\n",
           (ULONG)proof.initramfs_logical, (ULONG)proof.initramfs_physical,
           (ULONG)proof.initramfs_bytes);
    printf("M68KDEB_5U_TAKEOVER_IDENTITY_BOUND\n");
    printf("transition_entry_execute=NOT_ATTEMPTED\n");
    printf("mmu_mutation=NOT_ATTEMPTED\n");
    printf("cache_mutation=NOT_ATTEMPTED\n");
    printf("linux_jump=NOT_ATTEMPTED\n");
    printf("M68KDEB_5U_RETURNED\n");

    FreeMem(init_raw, INIT_RAW_BYTES);
    FreeMem(layout_raw, LAYOUT_RAW_BYTES);
    return 0;
}
