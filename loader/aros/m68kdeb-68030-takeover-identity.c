#include "m68kdeb-68030-takeover-identity.h"
#include "m68kdeb-68030-mmu-state.h"

static int span_inside(uint32_t base, uint32_t bytes,
                       uint32_t ram_base, uint32_t ram_bytes)
{
    uint32_t end, ram_end;
    if (!base || !bytes || !ram_bytes)
        return 0;
    if (base > 0xffffffffUL - bytes || ram_base > 0xffffffffUL - ram_bytes)
        return 0;
    end = base + bytes;
    ram_end = ram_base + ram_bytes;
    return base >= ram_base && end <= ram_end;
}

int m68kdeb_68030_takeover_identity_from_tc(
    uint32_t tc,
    const struct m68kdeb_takeover_request *req,
    uint32_t layout_ram_base,
    uint32_t layout_ram_bytes,
    uint32_t initramfs_ram_base,
    uint32_t initramfs_ram_bytes,
    struct m68kdeb_68030_takeover_identity_proof *out)
{
    uint32_t combined;

    if (!req || !out)
        return M68KDEB_68030_TAKEID_BAD_ARGUMENT;
    if (m68kdeb_takeover_68030_prepare(req) != M68KDEB_TAKEOVER_OK)
        return M68KDEB_68030_TAKEID_PREFLIGHT_FAILED;
    if (tc & (uint32_t)M68KDEB_68030_TC_ENABLE)
        return M68KDEB_68030_TAKEID_TRANSLATION_ENABLED;
    if ((uint32_t)req->layout_bytes > 0xffffffffUL - (uint32_t)req->bootinfo_bytes)
        return M68KDEB_68030_TAKEID_LAYOUT_OVERFLOW;
    combined = (uint32_t)req->layout_bytes + (uint32_t)req->bootinfo_bytes;
    if (!span_inside((uint32_t)req->layout_base, combined,
                     layout_ram_base, layout_ram_bytes))
        return M68KDEB_68030_TAKEID_LAYOUT_OUTSIDE_RAM;
    if ((uint32_t)req->initramfs_addr > 0xffffffffUL - (uint32_t)req->initramfs_bytes)
        return M68KDEB_68030_TAKEID_INITRAMFS_OVERFLOW;
    if (!span_inside((uint32_t)req->initramfs_addr,
                     (uint32_t)req->initramfs_bytes,
                     initramfs_ram_base, initramfs_ram_bytes))
        return M68KDEB_68030_TAKEID_INITRAMFS_OUTSIDE_RAM;

    out->layout_logical = (uint32_t)req->layout_base;
    out->layout_physical = (uint32_t)req->layout_base;
    out->layout_plus_bootinfo_bytes = combined;
    out->entry_logical = (uint32_t)req->entry_addr;
    out->entry_physical = (uint32_t)req->entry_addr;
    out->bootinfo_logical = (uint32_t)req->bootinfo_addr;
    out->bootinfo_physical = (uint32_t)req->bootinfo_addr;
    out->initramfs_logical = (uint32_t)req->initramfs_addr;
    out->initramfs_physical = (uint32_t)req->initramfs_addr;
    out->initramfs_bytes = (uint32_t)req->initramfs_bytes;
    return M68KDEB_68030_TAKEID_OK;
}
