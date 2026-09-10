#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <stdio.h>
#include <stdint.h>

#include "m68kdeb-68030-mmu-state.h"
#include "m68kdeb-68030-identity-window.h"
#include "m68kdeb-takeover.h"

#define PAGE_SIZE 4096UL
#define RAW_BYTES (PAGE_SIZE * 2UL - 1UL)

extern unsigned char m68kdeb_trampoline_blob[];
extern unsigned int m68kdeb_trampoline_blob_len;
extern unsigned int m68kdeb_trampoline_abi_probe_offset;
extern int m68kdeb_68030_call_transition_abi_probe(APTR probe,
    const struct m68kdeb_takeover_request *req, ULONG *capture);

static struct MemHeader *find_owner(struct ExecBase *SysBase, ULONG base, ULONG bytes)
{
    struct MemHeader *mh;
    ULONG end;
    if (!bytes || base > 0xffffffffUL - bytes) return NULL;
    end = base + bytes;
    for (mh = (struct MemHeader *)SysBase->MemList.lh_Head;
         mh->mh_Node.ln_Succ != NULL;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        if (base >= (ULONG)mh->mh_Lower && end <= (ULONG)mh->mh_Upper) return mh;
    }
    return NULL;
}

static int bytes_equal(const UBYTE *a, const UBYTE *b, ULONG n)
{
    ULONG i;
    for (i = 0; i < n; i++) if (a[i] != b[i]) return 0;
    return 1;
}

int main(void)
{
    struct ExecBase *SysBase = *(struct ExecBase **)4;
    UBYTE *raw, *page;
    struct MemHeader *owner;
    struct m68kdeb_physwin_candidate proof;
    struct m68kdeb_takeover_request req;
    ULONG capture[4] = {0,0,0,0};
    unsigned long tc = 0;
    APTR oldsp;
    ULONG blob_len = (ULONG)m68kdeb_trampoline_blob_len;
    ULONG probe_off = (ULONG)m68kdeb_trampoline_abi_probe_offset;
    int rc;

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("M68KDEB_5V_START\n");
    if (!blob_len || blob_len > PAGE_SIZE || probe_off >= blob_len) return 9;

    raw = (UBYTE *)AllocMem(RAW_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    if (!raw) return 10;
    page = (UBYTE *)(((ULONG)raw + PAGE_SIZE - 1UL) & ~(PAGE_SIZE - 1UL));
    owner = find_owner(SysBase, (ULONG)page, PAGE_SIZE);
    if (!owner) { FreeMem(raw, RAW_BYTES); return 11; }

    oldsp = SuperState();
    rc = m68kdeb_68030_read_tc(&tc);
    if (oldsp) UserState(oldsp);
    printf("tc=0x%08lx tc_rc=%ld\n", tc, (long)rc);
    if (rc != 0) { FreeMem(raw, RAW_BYTES); return 12; }

    rc = m68kdeb_68030_identity_window_from_tc((uint32_t)tc,
        (uint32_t)(ULONG)page, (uint32_t)PAGE_SIZE, (uint32_t)blob_len,
        (uint32_t)PAGE_SIZE, (uint32_t)(ULONG)owner->mh_Lower,
        (uint32_t)((ULONG)owner->mh_Upper - (ULONG)owner->mh_Lower), &proof);
    printf("identity_rc=%ld\n", (long)rc);
    if (rc != M68KDEB_68030_IDWIN_OK) { FreeMem(raw, RAW_BYTES); return 20; }

    CopyMem((APTR)m68kdeb_trampoline_blob, (APTR)page, blob_len);
    if (!bytes_equal(page, m68kdeb_trampoline_blob, blob_len)) {
        FreeMem(raw, RAW_BYTES); return 21;
    }

    req.layout_base = (ULONG)page;
    req.layout_bytes = 256UL;
    req.entry_addr = (ULONG)page + 64UL;
    req.bootinfo_addr = (ULONG)page + 256UL;
    req.bootinfo_bytes = 64UL;
    req.initramfs_addr = (ULONG)page + 512UL;
    req.initramfs_bytes = 128UL;
    req.cpu = M68KDEB_CPU_68030;
    req.mmu = M68KDEB_MMU_68030;

    printf("abi_probe_logical=0x%08lx abi_probe_physical=0x%08lx\n",
        (ULONG)proof.logical_base + probe_off,
        (ULONG)proof.physical_base + probe_off);
    printf("M68KDEB_5V_BEFORE_ABI_PROBE\n");
    rc = m68kdeb_68030_call_transition_abi_probe((APTR)(page + probe_off),
                                                  &req, capture);
    printf("abi_probe_rc=%ld\n", (long)rc);
    if (rc != 0 || capture[0] != (ULONG)&req || capture[1] != req.entry_addr ||
        capture[2] != req.bootinfo_addr || capture[3] != req.initramfs_addr) {
        printf("M68KDEB_5V_ABI_MISMATCH a0=0x%08lx a1=0x%08lx a2=0x%08lx a3=0x%08lx\n",
            capture[0], capture[1], capture[2], capture[3]);
        FreeMem(raw, RAW_BYTES); return 22;
    }

    printf("captured_a0=0x%08lx captured_a1=0x%08lx captured_a2=0x%08lx captured_a3=0x%08lx\n",
        capture[0], capture[1], capture[2], capture[3]);
    printf("M68KDEB_5V_TRANSITION_ABI_BOUND\n");
    printf("transition_entry_execute=NOT_ATTEMPTED\n");
    printf("mmu_mutation=NOT_ATTEMPTED\n");
    printf("cache_mutation=NOT_ATTEMPTED\n");
    printf("linux_jump=NOT_ATTEMPTED\n");
    printf("M68KDEB_5V_RETURNED\n");

    FreeMem(raw, RAW_BYTES);
    return 0;
}
