#ifndef M68KDEB_HANDOFF_H
#define M68KDEB_HANDOFF_H

#include <exec/types.h>

#include "m68kdeb-layout.h"
#include "m68kdeb-takeover.h"

int m68kdeb_build_takeover_request(const struct m68kdeb_final_layout *layout,
                                   ULONG bootinfo_bytes,
                                   ULONG initramfs_addr,
                                   ULONG initramfs_bytes,
                                   ULONG cpu,
                                   ULONG mmu,
                                   struct m68kdeb_takeover_request *req);

#endif
