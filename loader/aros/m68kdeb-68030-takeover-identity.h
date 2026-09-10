#ifndef M68KDEB_68030_TAKEOVER_IDENTITY_H
#define M68KDEB_68030_TAKEOVER_IDENTITY_H

#include <stdint.h>
#include "m68kdeb-takeover.h"

#define M68KDEB_68030_TAKEID_OK                    0
#define M68KDEB_68030_TAKEID_BAD_ARGUMENT          1
#define M68KDEB_68030_TAKEID_PREFLIGHT_FAILED      2
#define M68KDEB_68030_TAKEID_TRANSLATION_ENABLED   3
#define M68KDEB_68030_TAKEID_LAYOUT_OVERFLOW       4
#define M68KDEB_68030_TAKEID_LAYOUT_OUTSIDE_RAM    5
#define M68KDEB_68030_TAKEID_INITRAMFS_OVERFLOW    6
#define M68KDEB_68030_TAKEID_INITRAMFS_OUTSIDE_RAM 7

struct m68kdeb_68030_takeover_identity_proof {
    uint32_t layout_logical;
    uint32_t layout_physical;
    uint32_t layout_plus_bootinfo_bytes;
    uint32_t entry_logical;
    uint32_t entry_physical;
    uint32_t bootinfo_logical;
    uint32_t bootinfo_physical;
    uint32_t initramfs_logical;
    uint32_t initramfs_physical;
    uint32_t initramfs_bytes;
};

int m68kdeb_68030_takeover_identity_from_tc(
    uint32_t tc,
    const struct m68kdeb_takeover_request *req,
    uint32_t layout_ram_base,
    uint32_t layout_ram_bytes,
    uint32_t initramfs_ram_base,
    uint32_t initramfs_ram_bytes,
    struct m68kdeb_68030_takeover_identity_proof *out);

#endif
