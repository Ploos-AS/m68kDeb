/*
 * M1.3b.4b.5w actual-payload identity qualification harness.
 *
 * This translation unit includes the production loader unchanged and
 * interposes only m68kdeb_build_takeover_request().  That lets the normal
 * kernel -> final layout -> bootinfo -> initramfs -> takeover-request path run
 * exactly as in m68kdeb-loader.c, while adding a reversible MC68030 identity
 * proof before the loader reports its usual handoff=NOT_ATTEMPTED boundary.
 */

#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/exec.h>

#include "m68kdeb-handoff.h"
#include "m68kdeb-68030-mmu-state.h"
#include "m68kdeb-68030-takeover-identity.h"

static struct ExecBase *m68kdeb_5w_sysbase;

static int m68kdeb_5w_build_takeover_request(
    const struct m68kdeb_final_layout *layout,
    ULONG bootinfo_bytes,
    ULONG initramfs_addr,
    ULONG initramfs_bytes,
    ULONG cpu,
    ULONG mmu,
    struct m68kdeb_takeover_request *req);

#define m68kdeb_build_takeover_request m68kdeb_5w_build_takeover_request
#define main m68kdeb_5w_embedded_loader_main
#include "m68kdeb-loader.c"
#undef main
#undef m68kdeb_build_takeover_request

static struct MemHeader *m68kdeb_5w_owner(ULONG base, ULONG bytes)
{
    struct MemHeader *mh;
    ULONG end;

    if (!m68kdeb_5w_sysbase || !base || !bytes || base > 0xffffffffUL - bytes)
        return NULL;
    end = base + bytes;

    for (mh = (struct MemHeader *)m68kdeb_5w_sysbase->MemList.lh_Head;
         mh->mh_Node.ln_Succ != NULL;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        ULONG lower = (ULONG)mh->mh_Lower;
        ULONG upper = (ULONG)mh->mh_Upper;
        if (base >= lower && end <= upper)
            return mh;
    }
    return NULL;
}

static int m68kdeb_5w_build_takeover_request(
    const struct m68kdeb_final_layout *layout,
    ULONG bootinfo_bytes,
    ULONG initramfs_addr,
    ULONG initramfs_bytes,
    ULONG cpu,
    ULONG mmu,
    struct m68kdeb_takeover_request *req)
{
    struct m68kdeb_68030_takeover_identity_proof proof;
    struct MemHeader *layout_owner, *initramfs_owner;
    ULONG combined, tc = 0;
    APTR old_super;
    int rc, tc_rc, identity_rc;

    rc = m68kdeb_build_takeover_request(layout, bootinfo_bytes,
                                        initramfs_addr, initramfs_bytes,
                                        cpu, mmu, req);
    if (rc != M68KDEB_TAKEOVER_OK)
        return rc;

    if (req->layout_bytes > 0xffffffffUL - req->bootinfo_bytes) {
        Printf("M68KDEB_5W_ERROR combined_overflow\n");
        return M68KDEB_TAKEOVER_BAD_BOOTINFO;
    }
    combined = req->layout_bytes + req->bootinfo_bytes;
    layout_owner = m68kdeb_5w_owner(req->layout_base, combined);
    initramfs_owner = m68kdeb_5w_owner(req->initramfs_addr,
                                       req->initramfs_bytes);
    if (!layout_owner || !initramfs_owner) {
        Printf("M68KDEB_5W_ERROR owner_missing layout=%ld initramfs=%ld\n",
               (LONG)(layout_owner != NULL), (LONG)(initramfs_owner != NULL));
        return M68KDEB_TAKEOVER_BAD_INITRAMFS;
    }

    old_super = SuperState();
    tc_rc = m68kdeb_68030_read_tc(&tc);
    UserState(old_super);
    Printf("identity_tc_rc=%ld\n", (LONG)tc_rc);
    Printf("identity_tc=0x%08lx\n", tc);
    if (tc_rc != 0)
        return M68KDEB_TAKEOVER_UNSUPPORTED_CPU;

    identity_rc = m68kdeb_68030_takeover_identity_from_tc(
        (uint32_t)tc, req,
        (uint32_t)(ULONG)layout_owner->mh_Lower,
        (uint32_t)((ULONG)layout_owner->mh_Upper -
                   (ULONG)layout_owner->mh_Lower),
        (uint32_t)(ULONG)initramfs_owner->mh_Lower,
        (uint32_t)((ULONG)initramfs_owner->mh_Upper -
                   (ULONG)initramfs_owner->mh_Lower),
        &proof);

    Printf("takeover_identity_rc=%ld\n", (LONG)identity_rc);
    Printf("identity_layout_owner_lower=0x%08lx\n",
           (ULONG)layout_owner->mh_Lower);
    Printf("identity_layout_owner_upper=0x%08lx\n",
           (ULONG)layout_owner->mh_Upper);
    Printf("identity_initramfs_owner_lower=0x%08lx\n",
           (ULONG)initramfs_owner->mh_Lower);
    Printf("identity_initramfs_owner_upper=0x%08lx\n",
           (ULONG)initramfs_owner->mh_Upper);

    if (identity_rc != M68KDEB_68030_TAKEID_OK) {
        Printf("M68KDEB_5W_ERROR identity rc=%ld\n", (LONG)identity_rc);
        return M68KDEB_TAKEOVER_BAD_ALIGNMENT;
    }

    Printf("identity_layout_logical=0x%08lx\n", (ULONG)proof.layout_logical);
    Printf("identity_layout_physical=0x%08lx\n", (ULONG)proof.layout_physical);
    Printf("identity_entry_logical=0x%08lx\n", (ULONG)proof.entry_logical);
    Printf("identity_entry_physical=0x%08lx\n", (ULONG)proof.entry_physical);
    Printf("identity_bootinfo_logical=0x%08lx\n", (ULONG)proof.bootinfo_logical);
    Printf("identity_bootinfo_physical=0x%08lx\n", (ULONG)proof.bootinfo_physical);
    Printf("identity_initramfs_logical=0x%08lx\n", (ULONG)proof.initramfs_logical);
    Printf("identity_initramfs_physical=0x%08lx\n", (ULONG)proof.initramfs_physical);
    Printf("identity_layout_plus_bootinfo_bytes=%lu\n",
           (ULONG)proof.layout_plus_bootinfo_bytes);
    Printf("identity_initramfs_bytes=%lu\n", (ULONG)proof.initramfs_bytes);
    Printf("M68KDEB_TAKEOVER_IDENTITY_OK\n");
    return M68KDEB_TAKEOVER_OK;
}

int main(int argc, char **argv)
{
    m68kdeb_5w_sysbase = *((struct ExecBase **)4UL);
    Printf("M68KDEB_5W_ACTUAL_PAYLOAD_START\n");
    return m68kdeb_5w_embedded_loader_main(argc, argv);
}
