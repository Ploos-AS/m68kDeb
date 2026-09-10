/*
 * M1.3b.4b.5x actual-payload transition ABI qualification harness.
 *
 * The production loader source is included unchanged.  We interpose only the
 * completed takeover-request builder, then prove that the exact real payload
 * request is identity-mapped and that the copied reversible trampoline ABI
 * probe observes the same a0/a1/a2/a3 contract as the isolated final commit.
 */

#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/exec.h>

#include "m68kdeb-handoff.h"
#include "m68kdeb-68030-mmu-state.h"
#include "m68kdeb-68030-identity-window.h"
#include "m68kdeb-68030-takeover-identity.h"

#define PAGE_SIZE 4096UL
#define TRAMPOLINE_RAW_BYTES (PAGE_SIZE * 2UL - 1UL)

extern unsigned char m68kdeb_trampoline_blob[];
extern unsigned int m68kdeb_trampoline_blob_len;
extern unsigned int m68kdeb_trampoline_abi_probe_offset;
extern int m68kdeb_68030_call_transition_abi_probe(
    APTR probe, const struct m68kdeb_takeover_request *req, ULONG *capture);

static struct ExecBase *m68kdeb_5x_sysbase;
static int m68kdeb_5x_abi_bound;

static int m68kdeb_5x_build_takeover_request(
    const struct m68kdeb_final_layout *layout,
    ULONG bootinfo_bytes,
    ULONG initramfs_addr,
    ULONG initramfs_bytes,
    ULONG cpu,
    ULONG mmu,
    struct m68kdeb_takeover_request *req);

#define m68kdeb_build_takeover_request m68kdeb_5x_build_takeover_request
#define main m68kdeb_5x_embedded_loader_main
#include "m68kdeb-loader.c"
#undef main
#undef m68kdeb_build_takeover_request

static struct MemHeader *m68kdeb_5x_owner(ULONG base, ULONG bytes)
{
    struct MemHeader *mh;
    ULONG end;

    if (!m68kdeb_5x_sysbase || !base || !bytes || base > 0xffffffffUL - bytes)
        return NULL;
    end = base + bytes;
    for (mh = (struct MemHeader *)m68kdeb_5x_sysbase->MemList.lh_Head;
         mh->mh_Node.ln_Succ != NULL;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        ULONG lower = (ULONG)mh->mh_Lower;
        ULONG upper = (ULONG)mh->mh_Upper;
        if (base >= lower && end <= upper)
            return mh;
    }
    return NULL;
}

static int m68kdeb_5x_bytes_equal(const UBYTE *a, const UBYTE *b, ULONG bytes)
{
    ULONG i;
    for (i = 0; i < bytes; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

static int m68kdeb_5x_build_takeover_request(
    const struct m68kdeb_final_layout *layout,
    ULONG bootinfo_bytes,
    ULONG initramfs_addr,
    ULONG initramfs_bytes,
    ULONG cpu,
    ULONG mmu,
    struct m68kdeb_takeover_request *req)
{
    struct m68kdeb_68030_takeover_identity_proof payload_proof;
    struct m68kdeb_physwin_candidate trampoline_proof;
    struct MemHeader *layout_owner, *initramfs_owner, *trampoline_owner;
    UBYTE *trampoline_raw = NULL, *trampoline_page;
    ULONG capture[4] = {0, 0, 0, 0};
    ULONG combined, blob_len, probe_off;
    ULONG tc = 0;
    APTR old_super;
    int rc, tc_rc, identity_rc, abi_rc;

    rc = m68kdeb_build_takeover_request(layout, bootinfo_bytes,
                                        initramfs_addr, initramfs_bytes,
                                        cpu, mmu, req);
    Printf("M68KDEB_5X_REQUEST_BUILD_RC=%ld\n", (LONG)rc);
    if (rc != M68KDEB_TAKEOVER_OK)
        return rc;

    if (req->layout_bytes > 0xffffffffUL - req->bootinfo_bytes) {
        Printf("M68KDEB_5X_ERROR combined_overflow\n");
        return M68KDEB_TAKEOVER_BAD_BOOTINFO;
    }
    combined = req->layout_bytes + req->bootinfo_bytes;
    layout_owner = m68kdeb_5x_owner(req->layout_base, combined);
    initramfs_owner = m68kdeb_5x_owner(req->initramfs_addr,
                                       req->initramfs_bytes);
    if (!layout_owner || !initramfs_owner) {
        Printf("M68KDEB_5X_ERROR owner_missing layout=%ld initramfs=%ld\n",
               (LONG)(layout_owner != NULL),
               (LONG)(initramfs_owner != NULL));
        return M68KDEB_TAKEOVER_BAD_INITRAMFS;
    }

    old_super = SuperState();
    tc_rc = m68kdeb_68030_read_tc(&tc);
    if (old_super) UserState(old_super);
    Printf("abi_tc_rc=%ld\n", (LONG)tc_rc);
    Printf("abi_tc=0x%08lx\n", tc);
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
        &payload_proof);
    Printf("actual_payload_identity_rc=%ld\n", (LONG)identity_rc);
    if (identity_rc != M68KDEB_68030_TAKEID_OK)
        return M68KDEB_TAKEOVER_BAD_ALIGNMENT;

    blob_len = (ULONG)m68kdeb_trampoline_blob_len;
    probe_off = (ULONG)m68kdeb_trampoline_abi_probe_offset;
    if (!blob_len || blob_len > PAGE_SIZE || probe_off >= blob_len) {
        Printf("M68KDEB_5X_ERROR bad_trampoline_blob\n");
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    }

    trampoline_raw = (UBYTE *)AllocMem(TRAMPOLINE_RAW_BYTES,
                                        MEMF_PUBLIC | MEMF_CLEAR);
    if (!trampoline_raw) {
        Printf("M68KDEB_5X_ERROR trampoline_alloc\n");
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    }
    trampoline_page = (UBYTE *)(((ULONG)trampoline_raw + PAGE_SIZE - 1UL) &
                                ~(PAGE_SIZE - 1UL));
    trampoline_owner = m68kdeb_5x_owner((ULONG)trampoline_page, PAGE_SIZE);
    if (!trampoline_owner) {
        Printf("M68KDEB_5X_ERROR trampoline_owner\n");
        FreeMem(trampoline_raw, TRAMPOLINE_RAW_BYTES);
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    }

    rc = m68kdeb_68030_identity_window_from_tc(
        (uint32_t)tc,
        (uint32_t)(ULONG)trampoline_page,
        (uint32_t)PAGE_SIZE,
        (uint32_t)blob_len,
        (uint32_t)PAGE_SIZE,
        (uint32_t)(ULONG)trampoline_owner->mh_Lower,
        (uint32_t)((ULONG)trampoline_owner->mh_Upper -
                   (ULONG)trampoline_owner->mh_Lower),
        &trampoline_proof);
    Printf("trampoline_identity_rc=%ld\n", (LONG)rc);
    if (rc != M68KDEB_68030_IDWIN_OK) {
        FreeMem(trampoline_raw, TRAMPOLINE_RAW_BYTES);
        return M68KDEB_TAKEOVER_BAD_ALIGNMENT;
    }

    CopyMem((APTR)m68kdeb_trampoline_blob, (APTR)trampoline_page, blob_len);
    if (!m68kdeb_5x_bytes_equal(trampoline_page,
                                m68kdeb_trampoline_blob, blob_len)) {
        Printf("M68KDEB_5X_ERROR trampoline_copy_verify\n");
        FreeMem(trampoline_raw, TRAMPOLINE_RAW_BYTES);
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    }
    Printf("M68KDEB_5X_TRAMPOLINE_COPY_VERIFIED bytes=%lu\n", blob_len);
    Printf("abi_probe_logical=0x%08lx\n",
           (ULONG)trampoline_proof.logical_base + probe_off);
    Printf("abi_probe_physical=0x%08lx\n",
           (ULONG)trampoline_proof.physical_base + probe_off);

    abi_rc = m68kdeb_68030_call_transition_abi_probe(
        (APTR)(trampoline_page + probe_off), req, capture);
    Printf("actual_payload_abi_rc=%ld\n", (LONG)abi_rc);
    Printf("actual_req_ptr=0x%08lx\n", (ULONG)req);
    Printf("actual_req_entry=0x%08lx\n", req->entry_addr);
    Printf("actual_req_bootinfo=0x%08lx\n", req->bootinfo_addr);
    Printf("actual_req_initramfs=0x%08lx\n", req->initramfs_addr);
    Printf("captured_a0=0x%08lx\n", capture[0]);
    Printf("captured_a1=0x%08lx\n", capture[1]);
    Printf("captured_a2=0x%08lx\n", capture[2]);
    Printf("captured_a3=0x%08lx\n", capture[3]);

    if (abi_rc != 0 ||
        capture[0] != (ULONG)req ||
        capture[1] != req->entry_addr ||
        capture[2] != req->bootinfo_addr ||
        capture[3] != req->initramfs_addr) {
        Printf("M68KDEB_5X_ERROR abi_mismatch\n");
        FreeMem(trampoline_raw, TRAMPOLINE_RAW_BYTES);
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    }

    Printf("actual_layout_logical=0x%08lx\n",
           (ULONG)payload_proof.layout_logical);
    Printf("actual_layout_physical=0x%08lx\n",
           (ULONG)payload_proof.layout_physical);
    Printf("actual_entry_logical=0x%08lx\n",
           (ULONG)payload_proof.entry_logical);
    Printf("actual_entry_physical=0x%08lx\n",
           (ULONG)payload_proof.entry_physical);
    Printf("actual_bootinfo_logical=0x%08lx\n",
           (ULONG)payload_proof.bootinfo_logical);
    Printf("actual_bootinfo_physical=0x%08lx\n",
           (ULONG)payload_proof.bootinfo_physical);
    Printf("actual_initramfs_logical=0x%08lx\n",
           (ULONG)payload_proof.initramfs_logical);
    Printf("actual_initramfs_physical=0x%08lx\n",
           (ULONG)payload_proof.initramfs_physical);
    Printf("M68KDEB_5X_ACTUAL_PAYLOAD_ABI_BOUND\n");
    m68kdeb_5x_abi_bound = 1;
    FreeMem(trampoline_raw, TRAMPOLINE_RAW_BYTES);
    return M68KDEB_TAKEOVER_OK;
}

int main(int argc, char **argv)
{
    int rc;

    m68kdeb_5x_sysbase = *((struct ExecBase **)4UL);
    m68kdeb_5x_abi_bound = 0;
    Printf("M68KDEB_5X_START\n");
    rc = m68kdeb_5x_embedded_loader_main(argc, argv);
    if (rc == 0 && m68kdeb_5x_abi_bound) {
        Printf("transition_entry_execute=NOT_ATTEMPTED\n");
        Printf("mmu_mutation=NOT_ATTEMPTED\n");
        Printf("cache_mutation=NOT_ATTEMPTED\n");
        Printf("linux_jump=NOT_ATTEMPTED\n");
        Printf("M68KDEB_5X_RETURNED\n");
        return 0;
    }
    Printf("M68KDEB_5X_FAIL loader_rc=%ld abi_bound=%ld\n",
           (LONG)rc, (LONG)m68kdeb_5x_abi_bound);
    return rc ? rc : 20;
}
