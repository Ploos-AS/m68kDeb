/*
 * M1.3b.4b.6b CI-only Linux transfer runtime qualification harness.
 *
 * Production loader code is included unchanged. The completed takeover request
 * is intercepted only after the real kernel, bootinfo and initramfs have been
 * materialized. The harness repeats the exact payload identity and copied
 * trampoline gates, persists evidence, then executes the isolated 6a Linux
 * transfer trampoline. A return is always failure.
 */

#include <dos/dos.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "m68kdeb-handoff.h"
#include "m68kdeb-68030-mmu-state.h"
#include "m68kdeb-68030-identity-window.h"
#include "m68kdeb-68030-takeover-identity.h"

#define PAGE_SIZE 4096UL
#define TRAMPOLINE_RAW_BYTES (PAGE_SIZE * 2UL - 1UL)

extern unsigned char m68kdeb_linux_trampoline_blob[];
extern unsigned int m68kdeb_linux_trampoline_blob_len;
extern unsigned int m68kdeb_linux_trampoline_entry_offset;
extern unsigned int m68kdeb_linux_trampoline_mmu_off_offset;
extern unsigned int m68kdeb_linux_trampoline_post_mmu_offset;
extern unsigned int m68kdeb_linux_trampoline_jump_offset;
extern unsigned int m68kdeb_linux_trampoline_tc_zero_offset;
extern unsigned int m68kdeb_linux_trampoline_end_offset;
extern int m68kdeb_68030_jump_transition_entry(
    APTR entry, const struct m68kdeb_takeover_request *req);

static struct ExecBase *m68kdeb_6b_sysbase;

static int m68kdeb_6b_build_takeover_request(
    const struct m68kdeb_final_layout *layout,
    ULONG bootinfo_bytes,
    ULONG initramfs_addr,
    ULONG initramfs_bytes,
    ULONG cpu,
    ULONG mmu,
    struct m68kdeb_takeover_request *req);

#define m68kdeb_build_takeover_request m68kdeb_6b_build_takeover_request
#define main m68kdeb_6b_embedded_loader_main
#include "m68kdeb-loader.c"
#undef main
#undef m68kdeb_build_takeover_request

static struct MemHeader *m68kdeb_6b_owner(ULONG base, ULONG bytes)
{
    struct MemHeader *mh;
    ULONG end;

    if (!m68kdeb_6b_sysbase || !base || !bytes || base > 0xffffffffUL - bytes)
        return NULL;
    end = base + bytes;
    for (mh = (struct MemHeader *)m68kdeb_6b_sysbase->MemList.lh_Head;
         mh->mh_Node.ln_Succ != NULL;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
        ULONG lower = (ULONG)mh->mh_Lower;
        ULONG upper = (ULONG)mh->mh_Upper;
        if (base >= lower && end <= upper)
            return mh;
    }
    return NULL;
}

static int m68kdeb_6b_bytes_equal(const UBYTE *a, const UBYTE *b, ULONG bytes)
{
    ULONG i;
    for (i = 0; i < bytes; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

static int m68kdeb_6b_offsets_valid(ULONG blob_len)
{
    ULONG entry = (ULONG)m68kdeb_linux_trampoline_entry_offset;
    ULONG mmu_off = (ULONG)m68kdeb_linux_trampoline_mmu_off_offset;
    ULONG post_mmu = (ULONG)m68kdeb_linux_trampoline_post_mmu_offset;
    ULONG jump = (ULONG)m68kdeb_linux_trampoline_jump_offset;
    ULONG tc_zero = (ULONG)m68kdeb_linux_trampoline_tc_zero_offset;
    ULONG end = (ULONG)m68kdeb_linux_trampoline_end_offset;

    return entry == 0 && entry < mmu_off && mmu_off < post_mmu &&
           post_mmu < jump && jump < tc_zero && tc_zero + 4UL <= end &&
           end <= blob_len;
}

static int m68kdeb_6b_arm_marker(void)
{
    static const char text[] = "M1.3b.4b.6b Linux transfer armed\n";
    BPTR fh = Open("SYS:m1-3b4b6b-linux-transfer-armed.marker", MODE_NEWFILE);
    LONG wrote;

    if (!fh)
        return 1;
    wrote = Write(fh, (APTR)text, (LONG)(sizeof(text) - 1U));
    Close(fh);
    return wrote == (LONG)(sizeof(text) - 1U) ? 0 : 1;
}

static int m68kdeb_6b_build_takeover_request(
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
    UBYTE *trampoline_raw, *trampoline_page;
    ULONG combined, blob_len, entry_off, tc_zero_off;
    ULONG tc = 0;
    APTR old_super;
    int rc, tc_rc, identity_rc;

    rc = m68kdeb_build_takeover_request(layout, bootinfo_bytes,
                                        initramfs_addr, initramfs_bytes,
                                        cpu, mmu, req);
    Printf("M68KDEB_6B_REQUEST_BUILD_RC=%ld\n", (LONG)rc);
    if (rc != M68KDEB_TAKEOVER_OK)
        return rc;

    if (req->layout_bytes > 0xffffffffUL - req->bootinfo_bytes)
        return M68KDEB_TAKEOVER_BAD_BOOTINFO;
    combined = req->layout_bytes + req->bootinfo_bytes;
    layout_owner = m68kdeb_6b_owner(req->layout_base, combined);
    initramfs_owner = m68kdeb_6b_owner(req->initramfs_addr,
                                       req->initramfs_bytes);
    if (!layout_owner || !initramfs_owner)
        return M68KDEB_TAKEOVER_BAD_INITRAMFS;

    old_super = SuperState();
    tc_rc = m68kdeb_68030_read_tc(&tc);
    if (old_super) UserState(old_super);
    Printf("linux_transfer_tc_rc=%ld\n", (LONG)tc_rc);
    Printf("linux_transfer_tc=0x%08lx\n", tc);
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

    blob_len = (ULONG)m68kdeb_linux_trampoline_blob_len;
    entry_off = (ULONG)m68kdeb_linux_trampoline_entry_offset;
    tc_zero_off = (ULONG)m68kdeb_linux_trampoline_tc_zero_offset;
    if (!blob_len || blob_len > PAGE_SIZE || !m68kdeb_6b_offsets_valid(blob_len))
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;

    trampoline_raw = (UBYTE *)AllocMem(TRAMPOLINE_RAW_BYTES,
                                        MEMF_PUBLIC | MEMF_CLEAR);
    if (!trampoline_raw)
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    trampoline_page = (UBYTE *)(((ULONG)trampoline_raw + PAGE_SIZE - 1UL) &
                                ~(PAGE_SIZE - 1UL));
    trampoline_owner = m68kdeb_6b_owner((ULONG)trampoline_page, PAGE_SIZE);
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
    Printf("linux_trampoline_identity_rc=%ld\n", (LONG)rc);
    if (rc != M68KDEB_68030_IDWIN_OK) {
        FreeMem(trampoline_raw, TRAMPOLINE_RAW_BYTES);
        return M68KDEB_TAKEOVER_BAD_ALIGNMENT;
    }

    CopyMem((APTR)m68kdeb_linux_trampoline_blob,
            (APTR)trampoline_page, blob_len);
    if (!m68kdeb_6b_bytes_equal(trampoline_page,
                                m68kdeb_linux_trampoline_blob, blob_len) ||
        trampoline_page[tc_zero_off] != 0 ||
        trampoline_page[tc_zero_off + 1UL] != 0 ||
        trampoline_page[tc_zero_off + 2UL] != 0 ||
        trampoline_page[tc_zero_off + 3UL] != 0) {
        FreeMem(trampoline_raw, TRAMPOLINE_RAW_BYTES);
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    }

    Printf("M68KDEB_6B_TRAMPOLINE_COPY_VERIFIED bytes=%lu\n", blob_len);
    Printf("transition_entry_logical=0x%08lx\n",
           (ULONG)trampoline_proof.logical_base + entry_off);
    Printf("transition_entry_physical=0x%08lx\n",
           (ULONG)trampoline_proof.physical_base + entry_off);
    Printf("actual_entry_logical=0x%08lx\n", (ULONG)payload_proof.entry_logical);
    Printf("actual_entry_physical=0x%08lx\n", (ULONG)payload_proof.entry_physical);
    Printf("actual_bootinfo_logical=0x%08lx\n", (ULONG)payload_proof.bootinfo_logical);
    Printf("actual_bootinfo_physical=0x%08lx\n", (ULONG)payload_proof.bootinfo_physical);
    Printf("actual_initramfs_logical=0x%08lx\n", (ULONG)payload_proof.initramfs_logical);
    Printf("actual_initramfs_physical=0x%08lx\n", (ULONG)payload_proof.initramfs_physical);
    Printf("linux_jump=ARMED\n");
    Printf("commit_symbol=NOT_LINKED\n");
    Printf("M68KDEB_6B_LINUX_TRANSFER_ARMED\n");
    Flush(Output());

    if (m68kdeb_6b_arm_marker() != 0) {
        Printf("M68KDEB_6B_ERROR armed_marker\n");
        Flush(Output());
        FreeMem(trampoline_raw, TRAMPOLINE_RAW_BYTES);
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    }

    rc = m68kdeb_68030_jump_transition_entry(
        (APTR)(trampoline_page + entry_off), req);

    Printf("M68KDEB_6B_UNEXPECTED_RETURN rc=%ld\n", (LONG)rc);
    Flush(Output());
    FreeMem(trampoline_raw, TRAMPOLINE_RAW_BYTES);
    return M68KDEB_TAKEOVER_BAD_ARGUMENT;
}

int main(int argc, char **argv)
{
    int rc;

    m68kdeb_6b_sysbase = *((struct ExecBase **)4UL);
    Printf("M68KDEB_6B_START\n");
    rc = m68kdeb_6b_embedded_loader_main(argc, argv);
    Printf("M68KDEB_6B_LOADER_RETURNED_UNEXPECTEDLY rc=%ld\n", (LONG)rc);
    return rc ? rc : 20;
}
