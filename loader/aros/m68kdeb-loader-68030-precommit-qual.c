/*
 * M1.3b.4b.5y actual-payload pre-commit readiness qualification harness.
 *
 * This extends 5x without crossing the irreversible boundary.  The production
 * loader source is included unchanged.  We interpose only the completed
 * takeover-request builder, prove the exact real payload identity contract,
 * copy and verify the real transition trampoline into an identity-qualified
 * page, bind all critical transition symbols to logical==physical addresses,
 * and execute only the reversible ABI probe.  The real transition entry is
 * never called.
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
extern unsigned int m68kdeb_trampoline_entry_offset;
extern unsigned int m68kdeb_trampoline_mmu_off_offset;
extern unsigned int m68kdeb_trampoline_post_mmu_offset;
extern unsigned int m68kdeb_trampoline_tc_zero_offset;
extern unsigned int m68kdeb_trampoline_abi_probe_offset;
extern unsigned int m68kdeb_trampoline_reversible_probe_offset;
extern unsigned int m68kdeb_trampoline_end_offset;
extern int m68kdeb_68030_call_transition_abi_probe(
    APTR probe, const struct m68kdeb_takeover_request *req, ULONG *capture);

static struct ExecBase *m68kdeb_5y_sysbase;
static int m68kdeb_5y_precommit_ready;

static int m68kdeb_5y_build_takeover_request(
    const struct m68kdeb_final_layout *layout,
    ULONG bootinfo_bytes,
    ULONG initramfs_addr,
    ULONG initramfs_bytes,
    ULONG cpu,
    ULONG mmu,
    struct m68kdeb_takeover_request *req);

#define m68kdeb_build_takeover_request m68kdeb_5y_build_takeover_request
#define main m68kdeb_5y_embedded_loader_main
#include "m68kdeb-loader.c"
#undef main
#undef m68kdeb_build_takeover_request

static struct MemHeader *m68kdeb_5y_owner(ULONG base, ULONG bytes)
{
    struct MemHeader *mh;
    ULONG end;

    if (!m68kdeb_5y_sysbase || !base || !bytes || base > 0xffffffffUL - bytes)
        return NULL;
    end = base + bytes;
    for (mh = (struct MemHeader *)m68kdeb_5y_sysbase->MemList.lh_Head;
         mh->mh_Node.ln_Succ != NULL;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        ULONG lower = (ULONG)mh->mh_Lower;
        ULONG upper = (ULONG)mh->mh_Upper;
        if (base >= lower && end <= upper)
            return mh;
    }
    return NULL;
}

static int m68kdeb_5y_bytes_equal(const UBYTE *a, const UBYTE *b, ULONG bytes)
{
    ULONG i;
    for (i = 0; i < bytes; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

static int m68kdeb_5y_offsets_valid(ULONG blob_len)
{
    ULONG entry = (ULONG)m68kdeb_trampoline_entry_offset;
    ULONG mmu_off = (ULONG)m68kdeb_trampoline_mmu_off_offset;
    ULONG post_mmu = (ULONG)m68kdeb_trampoline_post_mmu_offset;
    ULONG tc_zero = (ULONG)m68kdeb_trampoline_tc_zero_offset;
    ULONG abi = (ULONG)m68kdeb_trampoline_abi_probe_offset;
    ULONG rev = (ULONG)m68kdeb_trampoline_reversible_probe_offset;
    ULONG end = (ULONG)m68kdeb_trampoline_end_offset;

    return entry == 0 &&
           entry < mmu_off && mmu_off < post_mmu && post_mmu < tc_zero &&
           tc_zero + 4UL <= abi && abi < rev && rev < end && end <= blob_len;
}

static int m68kdeb_5y_build_takeover_request(
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
    ULONG combined, blob_len, abi_off, tc_zero_off;
    ULONG entry_logical, entry_physical;
    ULONG tc = 0;
    APTR old_super;
    int rc, tc_rc, identity_rc, abi_rc;

    rc = m68kdeb_build_takeover_request(layout, bootinfo_bytes,
                                        initramfs_addr, initramfs_bytes,
                                        cpu, mmu, req);
    Printf("M68KDEB_5Y_REQUEST_BUILD_RC=%ld\n", (LONG)rc);
    if (rc != M68KDEB_TAKEOVER_OK)
        return rc;

    if (req->layout_bytes > 0xffffffffUL - req->bootinfo_bytes)
        return M68KDEB_TAKEOVER_BAD_BOOTINFO;
    combined = req->layout_bytes + req->bootinfo_bytes;
    layout_owner = m68kdeb_5y_owner(req->layout_base, combined);
    initramfs_owner = m68kdeb_5y_owner(req->initramfs_addr,
                                       req->initramfs_bytes);
    if (!layout_owner || !initramfs_owner)
        return M68KDEB_TAKEOVER_BAD_INITRAMFS;

    old_super = SuperState();
    tc_rc = m68kdeb_68030_read_tc(&tc);
    if (old_super) UserState(old_super);
    Printf("precommit_tc_rc=%ld\n", (LONG)tc_rc);
    Printf("precommit_tc=0x%08lx\n", tc);
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
    abi_off = (ULONG)m68kdeb_trampoline_abi_probe_offset;
    tc_zero_off = (ULONG)m68kdeb_trampoline_tc_zero_offset;
    if (!blob_len || blob_len > PAGE_SIZE || !m68kdeb_5y_offsets_valid(blob_len)) {
        Printf("M68KDEB_5Y_ERROR transition_layout\n");
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    }

    trampoline_raw = (UBYTE *)AllocMem(TRAMPOLINE_RAW_BYTES,
                                        MEMF_PUBLIC | MEMF_CLEAR);
    if (!trampoline_raw)
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    trampoline_page = (UBYTE *)(((ULONG)trampoline_raw + PAGE_SIZE - 1UL) &
                                ~(PAGE_SIZE - 1UL));
    trampoline_owner = m68kdeb_5y_owner((ULONG)trampoline_page, PAGE_SIZE);
    if (!trampoline_owner) {
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
    if (!m68kdeb_5y_bytes_equal(trampoline_page,
                                m68kdeb_trampoline_blob, blob_len)) {
        FreeMem(trampoline_raw, TRAMPOLINE_RAW_BYTES);
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    }
    if (trampoline_page[tc_zero_off] != 0 ||
        trampoline_page[tc_zero_off + 1UL] != 0 ||
        trampoline_page[tc_zero_off + 2UL] != 0 ||
        trampoline_page[tc_zero_off + 3UL] != 0) {
        Printf("M68KDEB_5Y_ERROR tc_zero_copy\n");
        FreeMem(trampoline_raw, TRAMPOLINE_RAW_BYTES);
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    }

    entry_logical = (ULONG)trampoline_proof.logical_base +
                    (ULONG)m68kdeb_trampoline_entry_offset;
    entry_physical = (ULONG)trampoline_proof.physical_base +
                     (ULONG)m68kdeb_trampoline_entry_offset;
    Printf("transition_entry_logical=0x%08lx\n", entry_logical);
    Printf("transition_entry_physical=0x%08lx\n", entry_physical);
    Printf("transition_mmu_off_logical=0x%08lx\n",
           (ULONG)trampoline_proof.logical_base +
           (ULONG)m68kdeb_trampoline_mmu_off_offset);
    Printf("transition_post_mmu_logical=0x%08lx\n",
           (ULONG)trampoline_proof.logical_base +
           (ULONG)m68kdeb_trampoline_post_mmu_offset);
    Printf("transition_tc_zero_logical=0x%08lx\n",
           (ULONG)trampoline_proof.logical_base + tc_zero_off);
    Printf("transition_abi_probe_logical=0x%08lx\n",
           (ULONG)trampoline_proof.logical_base + abi_off);
    Printf("transition_reversible_probe_logical=0x%08lx\n",
           (ULONG)trampoline_proof.logical_base +
           (ULONG)m68kdeb_trampoline_reversible_probe_offset);
    Printf("transition_end_logical=0x%08lx\n",
           (ULONG)trampoline_proof.logical_base +
           (ULONG)m68kdeb_trampoline_end_offset);
    if (entry_logical != entry_physical) {
        FreeMem(trampoline_raw, TRAMPOLINE_RAW_BYTES);
        return M68KDEB_TAKEOVER_BAD_ALIGNMENT;
    }
    Printf("M68KDEB_5Y_TRANSITION_LAYOUT_BOUND\n");

    abi_rc = m68kdeb_68030_call_transition_abi_probe(
        (APTR)(trampoline_page + abi_off), req, capture);
    Printf("actual_payload_abi_rc=%ld\n", (LONG)abi_rc);
    Printf("actual_req_ptr=0x%08lx\n", (ULONG)req);
    Printf("actual_req_entry=0x%08lx\n", req->entry_addr);
    Printf("actual_req_bootinfo=0x%08lx\n", req->bootinfo_addr);
    Printf("actual_req_initramfs=0x%08lx\n", req->initramfs_addr);
    Printf("captured_a0=0x%08lx\n", capture[0]);
    Printf("captured_a1=0x%08lx\n", capture[1]);
    Printf("captured_a2=0x%08lx\n", capture[2]);
    Printf("captured_a3=0x%08lx\n", capture[3]);
    if (abi_rc != 0 || capture[0] != (ULONG)req ||
        capture[1] != req->entry_addr || capture[2] != req->bootinfo_addr ||
        capture[3] != req->initramfs_addr) {
        FreeMem(trampoline_raw, TRAMPOLINE_RAW_BYTES);
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    }

    Printf("actual_layout_logical=0x%08lx\n", (ULONG)payload_proof.layout_logical);
    Printf("actual_layout_physical=0x%08lx\n", (ULONG)payload_proof.layout_physical);
    Printf("actual_entry_logical=0x%08lx\n", (ULONG)payload_proof.entry_logical);
    Printf("actual_entry_physical=0x%08lx\n", (ULONG)payload_proof.entry_physical);
    Printf("actual_bootinfo_logical=0x%08lx\n", (ULONG)payload_proof.bootinfo_logical);
    Printf("actual_bootinfo_physical=0x%08lx\n", (ULONG)payload_proof.bootinfo_physical);
    Printf("actual_initramfs_logical=0x%08lx\n", (ULONG)payload_proof.initramfs_logical);
    Printf("actual_initramfs_physical=0x%08lx\n", (ULONG)payload_proof.initramfs_physical);
    Printf("M68KDEB_5Y_PRECOMMIT_READY\n");
    m68kdeb_5y_precommit_ready = 1;
    FreeMem(trampoline_raw, TRAMPOLINE_RAW_BYTES);
    return M68KDEB_TAKEOVER_OK;
}

int main(int argc, char **argv)
{
    int rc;

    m68kdeb_5y_sysbase = *((struct ExecBase **)4UL);
    m68kdeb_5y_precommit_ready = 0;
    Printf("M68KDEB_5Y_START\n");
    rc = m68kdeb_5y_embedded_loader_main(argc, argv);
    if (rc == 0 && m68kdeb_5y_precommit_ready) {
        Printf("transition_entry_execute=NOT_ATTEMPTED\n");
        Printf("commit_execute=NOT_ATTEMPTED\n");
        Printf("mmu_mutation=NOT_ATTEMPTED\n");
        Printf("cache_mutation=NOT_ATTEMPTED\n");
        Printf("pflusha=NOT_ATTEMPTED\n");
        Printf("linux_jump=NOT_ATTEMPTED\n");
        Printf("M68KDEB_5Y_RETURNED\n");
        return 0;
    }
    Printf("M68KDEB_5Y_FAIL loader_rc=%ld ready=%ld\n",
           (LONG)rc, (LONG)m68kdeb_5y_precommit_ready);
    return rc ? rc : 20;
}
