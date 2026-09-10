#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <stdint.h>

#include "m68kdeb-handoff.h"
#include "m68kdeb-68030-mmu-state.h"
#include "m68kdeb-68030-takeover-identity.h"

static int m68kdeb_build_takeover_request_5w_hook(
    const struct m68kdeb_final_layout *layout,
    ULONG bootinfo_bytes,
    ULONG initramfs_addr,
    ULONG initramfs_bytes,
    ULONG cpu,
    ULONG mmu,
    struct m68kdeb_takeover_request *req);

/*
 * Compile the real loader implementation into this qualification binary and
 * intercept only the completed takeover request.  The loader algorithm itself
 * stays single-source.
 */
#define m68kdeb_build_takeover_request m68kdeb_build_takeover_request_5w_hook
#define main m68kdeb_loader_5w_embedded_main
#include "m68kdeb-loader.c"
#undef main
#undef m68kdeb_build_takeover_request

static int payload_identity_bound;

static struct MemHeader *find_owner(struct ExecBase *SysBase,
                                    ULONG base, ULONG bytes)
{
    struct MemHeader *mh;
    ULONG end;

    if (!SysBase || !base || !bytes || base > 0xffffffffUL - bytes)
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

static int m68kdeb_build_takeover_request_5w_hook(
    const struct m68kdeb_final_layout *layout,
    ULONG bootinfo_bytes,
    ULONG initramfs_addr,
    ULONG initramfs_bytes,
    ULONG cpu,
    ULONG mmu,
    struct m68kdeb_takeover_request *req)
{
    struct ExecBase *SysBase = *((struct ExecBase **)4UL);
    struct MemHeader *layout_owner, *init_owner;
    struct m68kdeb_68030_takeover_identity_proof proof;
    APTR oldsp;
    unsigned long tc = 0;
    ULONG combined;
    int rc, tc_rc, identity_rc;

    rc = m68kdeb_build_takeover_request(layout, bootinfo_bytes,
                                        initramfs_addr, initramfs_bytes,
                                        cpu, mmu, req);
    Printf("M68KDEB_5W_REQUEST_BUILD_RC=%ld\n", (LONG)rc);
    if (rc != M68KDEB_TAKEOVER_OK)
        return rc;

    if (!SysBase || req->layout_bytes > 0xffffffffUL - req->bootinfo_bytes) {
        Printf("M68KDEB_5W_OWNER_FAIL reason=layout_span\n");
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    }
    combined = req->layout_bytes + req->bootinfo_bytes;
    layout_owner = find_owner(SysBase, req->layout_base, combined);
    init_owner = find_owner(SysBase, req->initramfs_addr, req->initramfs_bytes);
    if (!layout_owner || !init_owner) {
        Printf("M68KDEB_5W_OWNER_FAIL layout=%ld initramfs=%ld\n",
               (LONG)(layout_owner != NULL), (LONG)(init_owner != NULL));
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    }

    Printf("M68KDEB_5W_LAYOUT_RAM lower=0x%08lx upper=0x%08lx\n",
           (ULONG)layout_owner->mh_Lower, (ULONG)layout_owner->mh_Upper);
    Printf("M68KDEB_5W_INITRAMFS_RAM lower=0x%08lx upper=0x%08lx\n",
           (ULONG)init_owner->mh_Lower, (ULONG)init_owner->mh_Upper);

    oldsp = SuperState();
    tc_rc = m68kdeb_68030_read_tc(&tc);
    if (oldsp) UserState(oldsp);
    Printf("M68KDEB_5W_TC=0x%08lx tc_rc=%ld\n", tc, (LONG)tc_rc);
    if (tc_rc != 0)
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;

    identity_rc = m68kdeb_68030_takeover_identity_from_tc(
        (uint32_t)tc, req,
        (uint32_t)(ULONG)layout_owner->mh_Lower,
        (uint32_t)((ULONG)layout_owner->mh_Upper - (ULONG)layout_owner->mh_Lower),
        (uint32_t)(ULONG)init_owner->mh_Lower,
        (uint32_t)((ULONG)init_owner->mh_Upper - (ULONG)init_owner->mh_Lower),
        &proof);
    Printf("M68KDEB_5W_IDENTITY_RC=%ld\n", (LONG)identity_rc);
    if (identity_rc != M68KDEB_68030_TAKEID_OK)
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;

    Printf("M68KDEB_5W_LAYOUT logical=0x%08lx physical=0x%08lx bytes=%lu\n",
           (ULONG)proof.layout_logical, (ULONG)proof.layout_physical,
           (ULONG)proof.layout_plus_bootinfo_bytes);
    Printf("M68KDEB_5W_ENTRY logical=0x%08lx physical=0x%08lx\n",
           (ULONG)proof.entry_logical, (ULONG)proof.entry_physical);
    Printf("M68KDEB_5W_BOOTINFO logical=0x%08lx physical=0x%08lx\n",
           (ULONG)proof.bootinfo_logical, (ULONG)proof.bootinfo_physical);
    Printf("M68KDEB_5W_INITRAMFS logical=0x%08lx physical=0x%08lx bytes=%lu\n",
           (ULONG)proof.initramfs_logical, (ULONG)proof.initramfs_physical,
           (ULONG)proof.initramfs_bytes);
    Printf("M68KDEB_5W_ACTUAL_PAYLOAD_IDENTITY_BOUND\n");
    payload_identity_bound = 1;
    return M68KDEB_TAKEOVER_OK;
}

int main(int argc, char **argv)
{
    int rc;

    Printf("M68KDEB_5W_START\n");
    payload_identity_bound = 0;
    rc = m68kdeb_loader_5w_embedded_main(argc, argv);
    if (rc == 0 && payload_identity_bound) {
        Printf("transition_entry_execute=NOT_ATTEMPTED\n");
        Printf("mmu_mutation=NOT_ATTEMPTED\n");
        Printf("cache_mutation=NOT_ATTEMPTED\n");
        Printf("linux_jump=NOT_ATTEMPTED\n");
        Printf("M68KDEB_5W_RETURNED\n");
        return 0;
    }
    Printf("M68KDEB_5W_FAIL loader_rc=%ld identity_bound=%ld\n",
           (LONG)rc, (LONG)payload_identity_bound);
    return rc ? rc : 20;
}
