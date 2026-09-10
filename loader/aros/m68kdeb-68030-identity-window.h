#ifndef M68KDEB_68030_IDENTITY_WINDOW_H
#define M68KDEB_68030_IDENTITY_WINDOW_H

#include <stdint.h>
#include "m68kdeb-physwin.h"

#define M68KDEB_68030_IDWIN_OK                  0
#define M68KDEB_68030_IDWIN_BAD_ARGUMENT        1
#define M68KDEB_68030_IDWIN_TRANSLATION_ENABLED 2
#define M68KDEB_68030_IDWIN_BAD_WINDOW          3

int m68kdeb_68030_identity_window_from_tc(
    uint32_t tc,
    uint32_t logical_base,
    uint32_t bytes,
    uint32_t trampoline_bytes,
    uint32_t alignment,
    uint32_t ram_base,
    uint32_t ram_bytes,
    struct m68kdeb_physwin_candidate *out);

#endif
