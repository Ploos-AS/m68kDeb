#ifndef M68KDEB_TAKEOVER_H
#define M68KDEB_TAKEOVER_H

#include <exec/types.h>

#define M68KDEB_TAKEOVER_OK                 0
#define M68KDEB_TAKEOVER_BAD_ARGUMENT       1
#define M68KDEB_TAKEOVER_BAD_ALIGNMENT      2
#define M68KDEB_TAKEOVER_BAD_ENTRY          3
#define M68KDEB_TAKEOVER_BAD_BOOTINFO       4
#define M68KDEB_TAKEOVER_BAD_INITRAMFS      5
#define M68KDEB_TAKEOVER_UNSUPPORTED_CPU    6

#define M68KDEB_CPU_68030 (1UL << 1)
#define M68KDEB_MMU_68030 (1UL << 1)

struct m68kdeb_takeover_request {
    ULONG layout_base;
    ULONG layout_bytes;
    ULONG entry_addr;
    ULONG bootinfo_addr;
    ULONG bootinfo_bytes;
    ULONG initramfs_addr;
    ULONG initramfs_bytes;
    ULONG cpu;
    ULONG mmu;
};

/*
 * Reversible boundary check.  This function must complete before any
 * supervisor takeover, interrupt masking, cache/MMU manipulation or jump.
 */
int m68kdeb_takeover_68030_prepare(const struct m68kdeb_takeover_request *req);

/*
 * M1.3b.4b.4 first irreversible boundary.  The implementation lives in the
 * isolated 68030 assembly object and is deliberately not linked into the real
 * loader yet.  Once called in supervisor mode it masks interrupts and does not
 * return.  DMA/cache/MMU handling and the Linux entry jump are still absent.
 */
void m68kdeb_takeover_68030_commit(const struct m68kdeb_takeover_request *req);

#endif
