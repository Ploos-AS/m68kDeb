#include "m68kdeb-takeover.h"

#define PAGE_SIZE 4096UL

static int add_overflows(ULONG a, ULONG b)
{
    return a > 0xffffffffUL - b;
}

int m68kdeb_takeover_68030_prepare(const struct m68kdeb_takeover_request *req)
{
    ULONG layout_end;

    if (!req || !req->layout_base || !req->layout_bytes ||
        !req->entry_addr || !req->bootinfo_addr || !req->bootinfo_bytes)
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;

    if ((req->layout_base & (PAGE_SIZE - 1UL)) != 0)
        return M68KDEB_TAKEOVER_BAD_ALIGNMENT;

    if (add_overflows(req->layout_base, req->layout_bytes))
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;
    layout_end = req->layout_base + req->layout_bytes;

    if (req->entry_addr < req->layout_base || req->entry_addr >= layout_end)
        return M68KDEB_TAKEOVER_BAD_ENTRY;

    /* Linux/m68k Amiga bootinfo must begin immediately after the image. */
    if (req->bootinfo_addr != layout_end)
        return M68KDEB_TAKEOVER_BAD_BOOTINFO;

    if (!req->initramfs_addr || !req->initramfs_bytes)
        return M68KDEB_TAKEOVER_BAD_INITRAMFS;

    /* M1.3b.4b starts with the qualified 68030 + on-chip MMU CI target. */
    if (req->cpu != M68KDEB_CPU_68030 || req->mmu != M68KDEB_MMU_68030)
        return M68KDEB_TAKEOVER_UNSUPPORTED_CPU;

    return M68KDEB_TAKEOVER_OK;
}
