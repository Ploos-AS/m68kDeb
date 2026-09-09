#include "m68kdeb-layout.h"
#include "m68kdeb-takeover.h"

int m68kdeb_build_takeover_request(const struct m68kdeb_final_layout *layout,
                                   ULONG bootinfo_bytes,
                                   ULONG initramfs_addr,
                                   ULONG initramfs_bytes,
                                   ULONG cpu,
                                   ULONG mmu,
                                   struct m68kdeb_takeover_request *req)
{
    if (!layout || !req || !layout->base || !layout->image_bytes ||
        !layout->entry_addr || !layout->bootinfo || !bootinfo_bytes ||
        bootinfo_bytes > layout->bootinfo_capacity ||
        !initramfs_addr || !initramfs_bytes)
        return M68KDEB_TAKEOVER_BAD_ARGUMENT;

    req->layout_base = (ULONG)layout->base;
    req->layout_bytes = layout->image_bytes;
    req->entry_addr = layout->entry_addr;
    req->bootinfo_addr = (ULONG)layout->bootinfo;
    req->bootinfo_bytes = bootinfo_bytes;
    req->initramfs_addr = initramfs_addr;
    req->initramfs_bytes = initramfs_bytes;
    req->cpu = cpu;
    req->mmu = mmu;

    return m68kdeb_takeover_68030_prepare(req);
}
