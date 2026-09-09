/*
 * M1.3b.4b.5d guarded irreversible 68030 takeover runtime harness.
 *
 * This is deliberately separate from m68kdeb-loader.  The normal loader
 * remains reversible.  This program exists only for the dedicated FS-UAE
 * qualification profile and never attempts a Linux entry transfer.
 */

#include <exec/memory.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "m68kdeb-takeover.h"

#define PAGE_SIZE 4096UL
#define ALLOC_BYTES (PAGE_SIZE * 4UL)

static ULONG align_page(ULONG p)
{
    return (p + PAGE_SIZE - 1UL) & ~(PAGE_SIZE - 1UL);
}

int main(void)
{
    UBYTE *raw;
    ULONG base;
    APTR old_sys_stack;
    struct m68kdeb_takeover_request req;
    int rc;

    PutStr("M68KDEB_5D_START\n");
    PutStr("scope=guarded irreversible 68030 commit qualification only\n");
    PutStr("linux_entry_transfer=NOT_IMPLEMENTED\n");

    raw = (UBYTE *)AllocMem(ALLOC_BYTES, MEMF_PUBLIC | MEMF_CLEAR);
    if (!raw) {
        PutStr("M68KDEB_5D_ERROR alloc\n");
        return 20;
    }

    base = align_page((ULONG)raw);
    req.layout_base = base;
    req.layout_bytes = PAGE_SIZE;
    req.entry_addr = base;
    req.bootinfo_addr = base + PAGE_SIZE;
    req.bootinfo_bytes = 64UL;
    req.initramfs_addr = base + (PAGE_SIZE * 2UL);
    req.initramfs_bytes = PAGE_SIZE;
    req.cpu = M68KDEB_CPU_68030;
    req.mmu = M68KDEB_MMU_68030;

    rc = m68kdeb_takeover_68030_prepare(&req);
    Printf("takeover_preflight_rc=%ld\n", (LONG)rc);
    Printf("layout_base=0x%08lx\n", req.layout_base);
    Printf("entry_addr=0x%08lx\n", req.entry_addr);
    Printf("bootinfo_addr=0x%08lx\n", req.bootinfo_addr);
    Printf("initramfs_addr=0x%08lx\n", req.initramfs_addr);
    if (rc != M68KDEB_TAKEOVER_OK) {
        PutStr("M68KDEB_5D_ERROR preflight\n");
        FreeMem(raw, ALLOC_BYTES);
        return 20;
    }

    PutStr("M68KDEB_TAKEOVER_PREFLIGHT_OK\n");
    PutStr("M68KDEB_5D_ARMED\n");
    PutStr("next=SuperState_then_m68kdeb_takeover_68030_commit\n");
    Flush(Output());

    /*
     * From here on there must be no Exec/DOS calls.  SuperState() keeps the
     * user stack accessible while entering supervisor state.  The qualified
     * commit path masks interrupts, disables 68030 cache/MMU state and loops;
     * it is intentionally non-returning in 5d.
     */
    old_sys_stack = SuperState();
    (void)old_sys_stack;
    m68kdeb_takeover_68030_commit(&req);

    /* Unreachable by contract. */
    return 20;
}
